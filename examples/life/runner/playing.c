#include "arviss.h"
#include "bitcasts.h"
#include "loadelf.h"
#include "mem.h"
#include "raylib.h"
#include "screens.h"

#include <stdbool.h>
#include <string.h>

#define NUM_COLS 64
#define NUM_ROWS 64

#define TILE_SIZE 12

#define BOARD_WIDTH (TILE_SIZE * NUM_COLS)
#define BOARD_HEIGHT (TILE_SIZE * NUM_ROWS)

#define BOARD_LEFT ((SCREEN_WIDTH - BOARD_WIDTH) / 2)
#define BOARD_TOP ((SCREEN_HEIGHT - BOARD_HEIGHT) / 2)

#define QUANTUM 1024

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

static bool board[2][NUM_ROWS * NUM_COLS];
static int current = 0;
static int next = 1;
static Guest guests[NUM_ROWS * NUM_COLS];

static const uint32_t membase = MEMBASE;
static const uint32_t memsize = MEMSIZE;
static const uint32_t rambase = RAMBASE;
static const uint32_t ramsize = RAMSIZE;

static int CountNeighbours(int row, int col);
static inline void SetNext(int row, int col, bool value);
static inline bool GetCurrent(int row, int col);

static uint8_t Read8(BusToken token, uint32_t addr, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= membase && addr < membase + memsize)
    {
        return memory->mem[addr - membase];
    }

    *busCode = bcLOAD_ACCESS_FAULT;
    return 0;
}

static uint16_t Read16(BusToken token, uint32_t addr, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= membase && addr < membase + memsize - 1)
    {
        // TODO: implement for big-endian ISAs.
        const uint16_t* base = (uint16_t*)&memory->mem[addr - membase];
        return *base;
    }

    *busCode = bcLOAD_ACCESS_FAULT;
    return 0;
}

static uint32_t Read32(BusToken token, uint32_t addr, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= membase && addr < membase + memsize - 3)
    {
        // TODO: implement for big-endian ISAs.
        const uint32_t* base = (uint32_t*)&memory->mem[addr - membase];
        return *base;
    }

    *busCode = bcLOAD_ACCESS_FAULT;
    return 0;
}

static void Write8(BusToken token, uint32_t addr, uint8_t byte, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= rambase && addr < rambase + ramsize)
    {
        memory->mem[addr - membase] = byte;
        return;
    }
    *busCode = bcSTORE_ACCESS_FAULT;
}

static void Write16(BusToken token, uint32_t addr, uint16_t halfword, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= rambase && addr < rambase + ramsize - 1)
    {
        // TODO: implement for big-endian ISAs.
        uint16_t* base = (uint16_t*)&memory->mem[addr - membase];
        *base = halfword;
        return;
    }

    *busCode = bcSTORE_ACCESS_FAULT;
}

static void Write32(BusToken token, uint32_t addr, uint32_t word, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= rambase && addr < rambase + ramsize - 2)
    {
        // TODO: implement for big-endian ISAs.
        uint32_t* base = (uint32_t*)&memory->mem[addr - membase];
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

static void InitGuest(Guest* guest)
{
    guest->bad = false;
    ArvissInit(&guest->cpu,
               &(Bus){.token = {&guest->memory},
                      .Read8 = Read8,
                      .Read16 = Read16,
                      .Read32 = Read32,
                      .Write8 = Write8,
                      .Write16 = Write16,
                      .Write32 = Write32});

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

static void PopulateBoard(void)
{
    for (int row = 0; row < NUM_ROWS; row++)
    {
        for (int col = 0; col < NUM_COLS; col++)
        {
            SetCurrent(row, col, GetRandomValue(0, 999) < 250);
            InitGuest(&guests[row * NUM_COLS + col]);
        }
    }
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

static void UpdateBoard(void)
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
            int x = startX + (col * TILE_SIZE);
            if (GetCurrent(row, col))
            {
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

void EnterPlaying(void)
{
    PopulateBoard();
}

void UpdatePlaying(void)
{
    UpdateBoard();
}

void UpdateFramePlaying(double elapsed)
{
}

void DrawPlaying(double alpha)
{
    (void)alpha;

    ClearBackground(BROWN);
    BeginDrawing();
    DrawBoard();
    DrawRectangle(0, 0, SCREEN_WIDTH, 32, GOLD);
    DrawText("Playing", 4, 4, 20, BLACK);
    DrawRectangle(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, 32, LIGHTGRAY);
    DrawFPS(4, SCREEN_HEIGHT - 20);
    EndDrawing();
}

void CheckTriggersPlaying(void)
{
    if (IsKeyPressed(KEY_ESCAPE))
    {
        SwitchTo(MENU);
    }
}
