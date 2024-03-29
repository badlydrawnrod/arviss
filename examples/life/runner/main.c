#define PHYSICS_FPS 60.0
#define BDR_LOOP_STATIC
#define BDR_LOOP_FIXED_UPDATE_INTERVAL_SECONDS (1.0 / PHYSICS_FPS)
#define BDR_LOOP_FIXED_UPDATE Update
#define BDR_LOOP_DRAW Draw
#define BDR_LOOP_IMPLEMENTATION

#include "arviss.h"
#include "bitcasts.h"
#include "game_loop.h"
#include "loadelf.h"
#include "mem.h"
#include "raylib.h"
#include "raymath.h"

#include <stdbool.h>
#include <string.h>

#define TARGET_FPS 60

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 1024

#define NUM_COLS 64
#define NUM_ROWS 64

#define TILE_SIZE 14

#define BOARD_WIDTH (TILE_SIZE * NUM_COLS)
#define BOARD_HEIGHT (TILE_SIZE * NUM_ROWS)

#define BOARD_LEFT ((SCREEN_WIDTH - BOARD_WIDTH) / 2)
#define BOARD_TOP ((SCREEN_HEIGHT - BOARD_HEIGHT) / 2)

#define QUANTUM 1024

#define MIN_UPDATE_RATE 0
#define MAX_UPDATE_RATE 60

typedef enum Syscalls
{
    SYSCALL_COUNT,     // Count neighbours.
    SYSCALL_GET_STATE, // Gets the state of this cell.
    SYSCALL_SET_STATE  // Sets the state of this cell.
} Syscalls;

typedef struct Guest
{
    ArvissCpu cpu;
    Memory memory;
    bool bad;
} Guest;

typedef struct ButtonInfo
{
    Rectangle rect;
    const char* text;
    Color normalBackground;
    Color hoverBackground;
    Color normalText;
    Color hoverText;
} ButtonInfo;

static bool board[2][NUM_ROWS * NUM_COLS];
static int current = 0;
static int next = 1;

static Guest guests[NUM_ROWS * NUM_COLS];

static float updateRate = MAX_UPDATE_RATE / 2.0f;
static bool isPaused = false;

static inline void SetNext(int row, int col, bool value)
{
    board[next][row * NUM_COLS + col] = value;
}

static inline void SetCurrent(int row, int col, bool value)
{
    board[current][row * NUM_COLS + col] = value;
}

static inline bool GetCurrent(int row, int col)
{
    return board[current][row * NUM_COLS + col];
}

static int CountNeighbours(int row, int col)
{
    int total = 0;
    for (int y = row - 1; y < row + 2; y++)
    {
        for (int x = col - 1; x < col + 2; x++)
        {
            if ((x != col) || (y != row))
            {
                if (y >= 0 && x >= 0 && y < NUM_ROWS && x < NUM_COLS)
                {
                    if (GetCurrent(y, x))
                    {
                        ++total;
                    }
                }
            }
        }
    }
    return total;
}

static void PopulateBoard(void)
{
    for (int row = 0; row < NUM_ROWS; row++)
    {
        for (int col = 0; col < NUM_COLS; col++)
        {
            SetCurrent(row, col, GetRandomValue(0, 999) < 250);
        }
    }
}

static inline void SysCountNeighbours(Guest* guest, int row, int col)
{
    const int neighbours = CountNeighbours(row, col);
    ArvissWriteXReg(&guest->cpu, abiA0, (uint32_t)neighbours);
}

static inline void SysGetState(Guest* guest, int row, int col)
{
    const bool isAlive = GetCurrent(row, col);
    ArvissWriteXReg(&guest->cpu, abiA0, BoolAsU32(isAlive));
}

static inline void SysSetState(Guest* guest, int row, int col)
{
    const bool isAlive = ArvissReadXReg(&guest->cpu, abiA0) != 0;
    SetNext(row, col, isAlive);
}

static void HandleTrap(Guest* guest, const ArvissTrap* trap, int row, int col)
{
    // Check for a syscall.
    if (trap->mcause == trENVIRONMENT_CALL_FROM_M_MODE)
    {
        // The syscall number is in a7 (x17).
        const uint32_t syscall = ArvissReadXReg(&guest->cpu, abiA7);

        // Service the syscall.
        bool syscallHandled = true;
        switch (syscall)
        {
        case SYSCALL_COUNT:
            SysCountNeighbours(guest, row, col);
            break;
        case SYSCALL_GET_STATE:
            SysGetState(guest, row, col);
            break;
        case SYSCALL_SET_STATE:
            SysSetState(guest, row, col);
            break;
        default:
            // Unknown syscall.
            TraceLog(LOG_WARNING, "Unknown syscall %04x", syscall);
            syscallHandled = false;
            guest->bad = true;
            break;
        }

        // If we handled the syscall then perform an MRET so that we can return from the trap.
        if (syscallHandled)
        {
            ArvissMret(&guest->cpu);
        }
    }
    else if (trap->mcause == trILLEGAL_INSTRUCTION)
    {
        TraceLog(LOG_WARNING, "Illegal instruction %8x", trap->mtval);
        guest->bad = true;
    }
    else
    {
        // Ideally I'd like this to be an error, but LOG_ERROR in raylib actually stops the program.
        TraceLog(LOG_WARNING, "Trap cause %d not recognised", trap->mcause);
        guest->bad = true;
    }
}

static void UpdateGuest(Guest* guest, int row, int col)
{
    int remaining = QUANTUM;
    while (!guest->bad && remaining > 0)
    {
        ArvissResult result = ArvissRun(&guest->cpu, remaining);
        if (ArvissResultIsTrap(result))
        {
            const ArvissTrap trap = ArvissResultAsTrap(result);
            HandleTrap(guest, &trap, row, col);
            if (trap.mcause == trENVIRONMENT_CALL_FROM_M_MODE && ArvissReadXReg(&guest->cpu, abiA7) == SYSCALL_SET_STATE)
            {
                // The VM gives up control whenever it sets the state of a cell.
                break;
            }
        }
        remaining -= guest->cpu.retired;
    }
}

static void Update(void)
{
    for (int row = 0; row < NUM_ROWS; row++)
    {
        for (int col = 0; col < NUM_COLS; col++)
        {
            Guest* guest = &guests[row * NUM_COLS + col];
            UpdateGuest(guest, row, col);
        }
    }
    int t = current;
    current = next;
    next = t;
}

static uint8_t Read8(BusToken token, uint32_t addr, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= MEMBASE && addr < MEMBASE + MEMSIZE)
    {
        return memory->mem[addr - MEMBASE];
    }
    *busCode = bcLOAD_ACCESS_FAULT;
    return 0;
}

static uint16_t Read16(BusToken token, uint32_t addr, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= MEMBASE && addr < MEMBASE + MEMSIZE - 1)
    {
        const uint16_t* base = (uint16_t*)&memory->mem[addr - MEMBASE]; // TODO: implement for big-endian ISAs.
        return *base;
    }
    *busCode = bcLOAD_ACCESS_FAULT;
    return 0;
}

static uint32_t Read32(BusToken token, uint32_t addr, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= MEMBASE && addr < MEMBASE + MEMSIZE - 3)
    {
        const uint32_t* base = (uint32_t*)&memory->mem[addr - MEMBASE]; // TODO: implement for big-endian ISAs.
        return *base;
    }
    *busCode = bcLOAD_ACCESS_FAULT;
    return 0;
}

static void Write8(BusToken token, uint32_t addr, uint8_t byte, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= RAMBASE && addr < RAMBASE + RAMSIZE)
    {
        memory->mem[addr - MEMBASE] = byte;
        return;
    }
    *busCode = bcSTORE_ACCESS_FAULT;
}

static void Write16(BusToken token, uint32_t addr, uint16_t halfword, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= RAMBASE && addr < RAMBASE + RAMSIZE - 1)
    {
        uint16_t* base = (uint16_t*)&memory->mem[addr - MEMBASE]; // TODO: implement for big-endian ISAs.
        *base = halfword;
        return;
    }
    *busCode = bcSTORE_ACCESS_FAULT;
}

static void Write32(BusToken token, uint32_t addr, uint32_t word, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= RAMBASE && addr < RAMBASE + RAMSIZE - 3)
    {
        uint32_t* base = (uint32_t*)&memory->mem[addr - MEMBASE]; // TODO: implement for big-endian ISAs.
        *base = word;
        return;
    }
    *busCode = bcSTORE_ACCESS_FAULT;
}

static void ZeroMem(ElfToken token, uint32_t addr, uint32_t len)
{
    uint8_t* target = token.t;
    memset(target + addr, 0, len);
}

static void WriteV(ElfToken token, uint32_t addr, void* src, uint32_t len)
{
    uint8_t* target = token.t;
    memcpy(target + addr, src, len);
}

static void InitGuests(void)
{
    for (int i = 0; i < NUM_ROWS * NUM_COLS; i++)
    {
        Guest* guest = &guests[i];
        guest->bad = false;
        ArvissInit(&guest->cpu,
                   &(Bus){.token = {&guest->memory},
                          .Read8 = Read8,
                          .Read16 = Read16,
                          .Read32 = Read32,
                          .Write8 = Write8,
                          .Write16 = Write16,
                          .Write32 = Write32});

        if (i == 0)
        {
            // Load the first guest.
            const char* filename = "../../../../examples/life/arviss/bin/cell";
            if (LoadElf(filename,
                        &(ElfLoaderConfig){.token = {&guest->memory.mem},
                                           .zeroMemFn = ZeroMem,
                                           .writeMemFn = WriteV,
                                           .targetSegments = (ElfSegmentDescriptor[]){{.start = ROM_START, .size = ROMSIZE},
                                                                                      {.start = RAMBASE, .size = RAMSIZE}},
                                           .numSegments = 2}))
            {
                TraceLog(LOG_WARNING, "--- Failed to load %s", filename);
            }
        }
        else
        {
            // Clone the remaining guests from the first.
            memcpy(&guest->memory.mem, &guests[0].memory.mem, MEMSIZE);
        }
    }
}

static void DrawBoard(void)
{
    int startX = BOARD_LEFT;
    int startY = BOARD_TOP;
    int endX = BOARD_LEFT + BOARD_WIDTH;
    int endY = BOARD_TOP + BOARD_HEIGHT;

    // Draw the background.
    DrawRectangle(startX, startY, BOARD_WIDTH, BOARD_HEIGHT, BEIGE);

    // Draw the content.
    for (int row = 0; row < NUM_ROWS; row++)
    {
        int y = startY + (row * TILE_SIZE);
        for (int col = 0; col < NUM_COLS; col++)
        {
            if (GetCurrent(row, col))
            {
                int x = startX + (col * TILE_SIZE);
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, LIME);
            }
        }
    }

    // Draw the grid.
    for (int row = 0; row <= NUM_ROWS; row++)
    {
        int y = startY + (row * TILE_SIZE);
        DrawLine(startX, y, endX, y, DARKBROWN);
    }
    for (int col = 0; col <= NUM_COLS; col++)
    {
        int x = startX + (col * TILE_SIZE);
        DrawLine(x, startY, x, endY, DARKBROWN);
    }
}

static bool Button(const ButtonInfo* info)
{
    Vector2 mousePos = GetMousePosition();
    bool mouseOver = CheckCollisionPointRec(mousePos, info->rect);
    DrawRectangleRec(info->rect, mouseOver ? info->hoverBackground : info->normalBackground);
    DrawText(info->text, (int)info->rect.x + 4, (int)info->rect.y + 6, 20, mouseOver ? info->hoverText : info->normalText);

    return mouseOver && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
}

static inline bool ResetButtonClicked(void)
{
    return Button(&(ButtonInfo){.rect = {BOARD_LEFT, SCREEN_HEIGHT - 60, 64, 32},
                                .text = "Reset",
                                .normalBackground = LIME,
                                .hoverBackground = RED,
                                .normalText = BLACK,
                                .hoverText = WHITE});
}

static inline bool PauseButtonClicked(void)
{
    return Button(&(ButtonInfo){.rect = {BOARD_LEFT + BOARD_WIDTH - 72, SCREEN_HEIGHT - 60, 72, 32},
                                .text = isPaused ? "Go" : "Stop",
                                .normalBackground = LIME,
                                .hoverBackground = DARKGREEN,
                                .normalText = BLACK,
                                .hoverText = BLACK});
}

static float GetUpdateRate(Rectangle rect, float currentValue, float minValue, float maxValue)
{
    Vector2 mousePos = GetMousePosition();
    float range = rect.width - 16;

    bool mouseOver = CheckCollisionPointRec(mousePos, rect);
    if (mouseOver && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
        Vector2 bar = {mousePos.x, rect.y};
        bar.x = (bar.x - 8 < rect.x) ? rect.x + 8 : bar.x;
        bar.x = (bar.x + 8 > rect.x + rect.width) ? rect.x + rect.width - 8 : bar.x;
        float normalised = (bar.x - (rect.x + 8)) / range;
        currentValue = (maxValue - minValue) * normalised + minValue;
    }

    float where = (currentValue - minValue) / (maxValue - minValue);
    float barX = rect.x + 8 + where * range;

    DrawRectangleRec(rect, mouseOver ? DARKGREEN : LIME);
    DrawRectangle((int)barX - 8, (int)rect.y, 16, (int)rect.height, GREEN);

    const char* speed = TextFormat("Rate: %2.1f Hz", currentValue);
    DrawText(speed, (int)rect.x + 4, (int)rect.y + 6, 20, BLACK);

    return currentValue;
}

static void CheckForBoardClick(void)
{
    const Vector2 mousePos = GetMousePosition();
    Rectangle rect = {.x = BOARD_LEFT, .y = BOARD_TOP, .width = BOARD_WIDTH, .height = BOARD_HEIGHT};
    if (CheckCollisionPointRec(mousePos, rect))
    {
        Vector2 boardPos = Vector2Subtract(mousePos, (Vector2){rect.x, rect.y});
        int col = (int)boardPos.x / TILE_SIZE;
        int row = (int)boardPos.y / TILE_SIZE;
        DrawRectangle((int)rect.x + col * TILE_SIZE, (int)rect.y + row * TILE_SIZE, TILE_SIZE, TILE_SIZE, RED);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            bool isAlive = GetCurrent(row, col);
            SetCurrent(row, col, !isAlive);
        }
    }
}

static void Draw(double alpha)
{
    (void)alpha;

    ClearBackground(BROWN);
    BeginDrawing();
    DrawBoard();
    DrawRectangle(0, 0, SCREEN_WIDTH, 32, GOLD);
    DrawText("Life", 4, 4, 20, BLACK);
    DrawRectangle(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, 32, LIGHTGRAY);
    DrawFPS(4, SCREEN_HEIGHT - 20);

    if (ResetButtonClicked())
    {
        PopulateBoard();
    }
    if (PauseButtonClicked())
    {
        isPaused = !isPaused;
    }
    updateRate = GetUpdateRate((Rectangle){BOARD_LEFT + 80, SCREEN_HEIGHT - 60, BOARD_WIDTH - (80 + 88), 32}, updateRate,
                               MIN_UPDATE_RATE, MAX_UPDATE_RATE);
    SetUpdateInterval(isPaused ? INFINITY : 1.0 / updateRate);
    CheckForBoardClick();

    EndDrawing();
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Life");
    SetTargetFPS(TARGET_FPS);
    SetTraceLogLevel(LOG_DEBUG);

    InitGuests();
    PopulateBoard();
    RunMainLoop();

    CloseWindow();

    return 0;
}
