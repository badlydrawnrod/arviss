#include "arviss.h"
#include "loadelf.h"
#include "mem.h"
#include "raylib.h"
#include "raymath.h"
#include "syscalls.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

// Perform up to 1024 instructions per update.
#define QUANTUM 1024

#define TURTLE_SCALE 12.0f
#define NUM_TURTLES 10
#define MAX_LINES 12
#define MAX_TURN (5.0f * DEG2RAD)
#define MAX_SPEED 1.0f

typedef struct VM
{
    ArvissCpu cpu;
    Memory memory;
    Bus bus;
    bool isBlocked;
} VM;

typedef struct Turtle
{
    VM vm;

    bool isActive;
    Vector2 lastPosition;
    Vector2 position;
    float heading;
    bool isVisible;
    bool isPenDown;
    Color penColour;
    float aheadRemaining;
    float turnRemaining;
} Turtle;

// Types of draw command.
typedef enum
{
    END,  // Indicates the last command.
    MOVE, // Move to a given position.
    LINE  // Draw a line from the current position to the given position. If there is no current position, start from the origin.
} CommandType;

// A draw command.
typedef struct
{
    CommandType type;
    Vector2 pos;
} DrawCommand;

// Turtle appearance.
static const DrawCommand turtleShape[MAX_LINES] = {{MOVE, {-1, 1}},   {LINE, {0, -1}}, {LINE, {1, 1}},
                                                   {LINE, {0, 0.5f}}, {LINE, {-1, 1}}, {END, {0, 0}}};

static void Home(Turtle* turtle);

// --- Bus access ------------------------------------------------------------------------------------------------------------------

static const uint32_t membase = MEMBASE;
static const uint32_t memsize = MEMSIZE;
static const uint32_t rambase = RAMBASE;
static const uint32_t ramsize = RAMSIZE;

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

// --- Initialization --------------------------------------------------------------------------------------------------------------

static void LoadCode(Memory* memory, const char* filename)
{
    printf("--- Loading %s\n", filename);
    MemoryDescriptor memoryDesc[] = {{.start = ROM_START, .size = ROMSIZE, .data = memory->mem + ROM_START},
                                     {.start = RAMBASE, .size = RAMSIZE, .data = memory->mem + RAMBASE}};
    if (LoadElf(filename, memoryDesc, sizeof(memoryDesc) / sizeof(memoryDesc[0])) != ER_OK)
    {
        printf("--- Failed to load %s\n", filename);
    }
}

static void InitTurtle(Turtle* turtle)
{
    turtle->vm.bus = (Bus){.token = {&turtle->vm.memory},
                           .Read8 = Read8,
                           .Read16 = Read16,
                           .Read32 = Read32,
                           .Write8 = Write8,
                           .Write16 = Write16,
                           .Write32 = Write32};

    ArvissInit(&turtle->vm.cpu, &turtle->vm.bus);
    turtle->vm.isBlocked = false;

    const char* filename = "../../../../examples/turtles/arviss/bin/turtle";
    Memory* memory = &turtle->vm.memory;
    LoadCode(memory, filename);

    turtle->isActive = true;

    Home(turtle);
}

static void InitTurtles(Turtle* turtles, int numTurtles)
{
    for (int i = 0; i < numTurtles; i++)
    {
        InitTurtle(&turtles[i]);
        turtles[i].heading = (float)i * DEG2RAD * 360.0f / numTurtles;
    }
}

static void DestroyTurtle(Turtle* turtle)
{
}

static void DestroyTurtles(Turtle* turtles, int numTurtles)
{
    for (int i = 0; i < numTurtles; i++)
    {
        DestroyTurtle(&turtles[i]);
    }
}

// --- Turtle commmands ------------------------------------------------------------------------------------------------------------

static void Home(Turtle* turtle)
{
    turtle->lastPosition = Vector2Zero();
    turtle->position = Vector2Zero();
    turtle->heading = 0.0f;
    turtle->isVisible = true;
    turtle->isPenDown = true;
    turtle->penColour = RAYWHITE;
    turtle->aheadRemaining = 0.0f;
    turtle->turnRemaining = 0.0f;
}

static void SetAhead(Turtle* turtle, float distance)
{
    turtle->aheadRemaining = distance;
    turtle->turnRemaining = 0.0f;
    turtle->vm.isBlocked = true;
}

static void SetTurn(Turtle* turtle, float angle)
{
    turtle->turnRemaining = angle * DEG2RAD;
    turtle->aheadRemaining = 0.0f;
    turtle->vm.isBlocked = true;
}

static void Goto(Turtle* turtle, float x, float y)
{
    // This effectively teleports to the given position.
    turtle->position.x = x;
    turtle->position.y = y;
}

static void SetPenState(Turtle* turtle, bool isDown)
{
    turtle->isPenDown = isDown;
}

static void SetVisibility(Turtle* turtle, bool isVisible)
{
    turtle->isVisible = isVisible;
}

static void SetPenColour(Turtle* turtle, Color penColour)
{
    turtle->penColour = penColour;
}

// --- Movement and turning --------------------------------------------------------------------------------------------------------

static inline float NormalizeAngle(float angle)
{
    // Get the positive modulus so that 0 <= angle < 2 * PI.
    angle = fmodf(angle, 2 * PI);
    angle = fmodf(angle + 2 * PI, 2 * PI);
    return angle;
}

static inline bool IsFZero(float a)
{
    return fabsf(a) < FLT_EPSILON;
}

static void MoveTurtle(Turtle* turtle)
{
    if (!turtle->isActive)
    {
        return;
    }

    turtle->lastPosition = turtle->position;

    if (!IsFZero(turtle->aheadRemaining))
    {
        // Move ahead (or back if negative).
        float magDistance = fminf(MAX_SPEED, fabsf(turtle->aheadRemaining));
        float distance = copysignf(magDistance, turtle->aheadRemaining);

        float angle = turtle->heading;
        Vector2 heading = (Vector2){sinf(angle), cosf(angle)};
        turtle->position = Vector2Add(turtle->position, Vector2Scale(heading, distance));
        turtle->aheadRemaining -= distance;
        turtle->vm.isBlocked = !IsFZero(turtle->aheadRemaining);
    }
    else if (!IsFZero(turtle->turnRemaining))
    {
        // Turn right (or left if negative).
        float magTurn = fminf(MAX_TURN, fabsf(turtle->turnRemaining));
        float turn = copysignf(magTurn, turtle->turnRemaining);
        turtle->heading += turn;
        turtle->heading = NormalizeAngle(turtle->heading);
        turtle->turnRemaining -= turn;
        turtle->vm.isBlocked = !IsFZero(turtle->turnRemaining);
    }
}

static void MoveTurtles(Turtle* turtles, int numTurtles)
{
    for (int i = 0; i < numTurtles; i++)
    {
        if (turtles[i].isActive)
        {
            MoveTurtle(&turtles[i]);
        }
    }
}

// --- Turtle VM -------------------------------------------------------------------------------------------------------------------

static void HandleTrap(Turtle* turtle, const ArvissTrap* trap)
{
    // Check for a syscall.
    if (trap->mcause == trENVIRONMENT_CALL_FROM_M_MODE)
    {
        // The syscall number is in a7 (x17).
        const uint32_t syscall = ArvissReadXReg(&turtle->vm.cpu, 17);

        // Service the syscall.
        bool syscallHandled = true;
        switch (syscall)
        {
        case SYSCALL_EXIT: {
            // The exit code is in a0 (x10).
            turtle->isActive = false;
        }
        break;
        case SYSCALL_HOME: {
            Home(turtle);
        }
        break;
        case SYSCALL_AHEAD: {
            // The distance is in a0 (x10).
            uint32_t a0 = ArvissReadXReg(&turtle->vm.cpu, 10);
            float distance = *(float*)&a0;
            SetAhead(turtle, distance);
        }
        break;
        case SYSCALL_TURN: {
            // The angle is in a0 (x10).
            uint32_t a0 = ArvissReadXReg(&turtle->vm.cpu, 10);
            float angle = *(float*)&a0;
            SetTurn(turtle, angle);
        }
        break;
        case SYSCALL_GOTO: {
            // The coordinates are in a0 and a1 (x10 and x11).
            uint32_t a0 = ArvissReadXReg(&turtle->vm.cpu, 10);
            uint32_t a1 = ArvissReadXReg(&turtle->vm.cpu, 11);
            float x = *(float*)&a0;
            float y = *(float*)&a1;
            Goto(turtle, x, y);
        }
        break;
        case SYSCALL_SET_PEN_STATE: {
            // The pen state is in a0 (x10).
            uint32_t a0 = ArvissReadXReg(&turtle->vm.cpu, 10);
            SetPenState(turtle, a0 != 0);
        }
        break;
        case SYSCALL_GET_PEN_STATE: {
            // Return the pen state in a0 (x10).
            ArvissWriteXReg(&turtle->vm.cpu, 10, turtle->isPenDown);
        }
        break;
        case SYSCALL_SET_VISIBILITY: {
            // The visibility state is in a0 (x10).
            uint32_t a0 = ArvissReadXReg(&turtle->vm.cpu, 10);
            SetVisibility(turtle, a0 != 0);
        }
        break;
        case SYSCALL_GET_VISIBILITY: {
            // Return the visibility state in a0 (x10).
            ArvissWriteXReg(&turtle->vm.cpu, 10, turtle->isVisible);
        }
        break;
        case SYSCALL_SET_PEN_COLOUR: {
            // The colour's RGBA components are given in a0 (x10).
            uint32_t a0 = ArvissReadXReg(&turtle->vm.cpu, 10);
            Color colour = GetColor((int)a0);
            SetPenColour(turtle, colour);
        }
        break;
        case SYSCALL_GET_PEN_COLOUR: {
            // Return the colour's RGBA components in a0 (x10).
            ArvissWriteXReg(&turtle->vm.cpu, 10, (uint32_t)ColorToInt(turtle->penColour));
        }
        break;
        case SYSCALL_GET_POSITION: {
            // Return the turtle's position via the addresses pointed to by a0 and a1 (x10 and x11). An address of zero means
            // that the given coordinate is not required.
            uint32_t a0 = ArvissReadXReg(&turtle->vm.cpu, 10);
            BusCode mc = bcOK;
            if (a0 != 0)
            {
                // Copy the x coordinate into memory at a0 (x10).
                Write32((BusToken){&turtle->vm.memory}, a0, *(uint32_t*)&turtle->position.x, &mc);
            }
            uint32_t a1 = ArvissReadXReg(&turtle->vm.cpu, 11);
            if (mc == bcOK && a1 != 0)
            {
                // Copy the y coordinate into memory at a1 (x11).
                Write32((BusToken){&turtle->vm.memory}, a1, *(uint32_t*)&turtle->position.y, &mc);
            }
            // Return success / failure in a0 (x10).
            ArvissWriteXReg(&turtle->vm.cpu, 10, (mc == bcOK));
        }
        break;
        case SYSCALL_GET_HEADING: {
            // Return the turtle's heading in a0 (x10).
            const float heading = turtle->heading * RAD2DEG;
            ArvissWriteXReg(&turtle->vm.cpu, 10, *(uint32_t*)&heading);
        }
        break;
        default:
            // Unknown syscall.
            syscallHandled = false;
            break;
        }

        // If we handled the syscall then perform an MRET so that we can return from the trap.
        if (syscallHandled)
        {
            ArvissMret(&turtle->vm.cpu);
        }
    }
}

static void UpdateTurtleVM(Turtle* turtle)
{
    if (turtle->vm.isBlocked)
    {
        return;
    }

    ArvissResult result = ArvissRun(&turtle->vm.cpu, QUANTUM);

    if (ArvissResultIsTrap(result))
    {
        const ArvissTrap trap = ArvissResultAsTrap(result);
        HandleTrap(turtle, &trap);
    }
}

static void UpdateTurtleVMs(Turtle* turtles, int numTurtles)
{
    for (int i = 0; i < numTurtles; i++)
    {
        if (!turtles[i].vm.isBlocked)
        {
            UpdateTurtleVM(&turtles[i]);
        }
    }
}

// --- Drawing ---------------------------------------------------------------------------------------------------------------------

static void DrawPens(Turtle* turtles, int numTurtles)
{
    for (int i = 0; i < numTurtles; i++)
    {
        if (turtles[i].isPenDown
            && (turtles[i].position.x != turtles[i].lastPosition.x || turtles[i].position.y != turtles[i].lastPosition.y))
        {
            DrawLineV(turtles[i].lastPosition, turtles[i].position, turtles[i].penColour);
        }
    }
}

static void DrawShape(const DrawCommand* commands, Vector2 pos, float heading, Color colour)
{
    Vector2 points[MAX_LINES];

    // Default to starting at the origin.
    pos.y = -pos.y;
    Vector2 here = Vector2Add(Vector2Scale(Vector2Rotate((Vector2){0, 0}, heading), TURTLE_SCALE), pos);

    int numPoints = 0;
    for (int i = 0; commands[i].type != END; i++)
    {
        const Vector2 coord = Vector2Add(Vector2Scale(Vector2Rotate(commands[i].pos, heading), TURTLE_SCALE), pos);
        if (commands[i].type == LINE)
        {
            if (numPoints == 0)
            {
                points[0] = here;
                ++numPoints;
            }
            points[numPoints] = coord;
            ++numPoints;
        }
        else if (commands[i].type == MOVE)
        {
            if (numPoints > 0)
            {
                DrawLineStrip(points, numPoints, colour);
                numPoints = 0;
            }
        }
        here = coord;
    }
    if (numPoints > 0)
    {
        DrawLineStrip(points, numPoints, colour);
    }
}

static void DrawTurtle(Turtle* turtle)
{
    if (turtle->isVisible)
    {
        DrawShape(turtleShape, turtle->position, turtle->heading * RAD2DEG, turtle->penColour);
    }
}

static void DrawTurtles(Turtle* turtles, int numTurtles)
{
    for (int i = 0; i < numTurtles; i++)
    {
        DrawTurtle(&turtles[i]);
    }
}

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Turtles");
    SetTargetFPS(60);

    // Create the canvas that the turtles will draw onto.
    RenderTexture2D canvas = LoadRenderTexture(screenWidth, screenHeight);
    BeginTextureMode(canvas);
    ClearBackground(BLACK);
    EndTextureMode();

    // Create a camera whose origin is at the centre of the canvas.
    Vector2 origin = (Vector2){.x = (float)(canvas.texture.width / 2), .y = (float)(canvas.texture.height / 2)};
    Camera2D camera = {.offset = origin, .zoom = 1.0f};

    static Turtle turtles[NUM_TURTLES]; // This is static to keep it off the stack.
    InitTurtles(turtles, NUM_TURTLES);

    while (!WindowShouldClose())
    {
        UpdateTurtleVMs(turtles, NUM_TURTLES);
        MoveTurtles(turtles, NUM_TURTLES);

        BeginDrawing();
        ClearBackground(DARKGRAY);

        // If any of the turtles have moved then add the last line that they made to the canvas.
        BeginTextureMode(canvas);
        BeginMode2D(camera);
        DrawPens(turtles, NUM_TURTLES);
        EndMode2D();
        EndTextureMode();

        // Draw the canvas that the turtles draw to.
        DrawTexture(canvas.texture, 0, 0, WHITE);

        // Draw the visible turtles above the canvas.
        BeginMode2D(camera);
        DrawTurtles(turtles, NUM_TURTLES);
        EndMode2D();

        EndDrawing();
    }
    CloseWindow();
    DestroyTurtles(turtles, NUM_TURTLES);

    return 0;
}
