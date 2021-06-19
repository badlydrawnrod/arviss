#include "arviss.h"
#include "mem.h"
#include "raylib.h"
#include "raymath.h"
#include "syscalls.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

// Do 1024 instructions per update.
#define QUANTUM 1024

#define TURTLE_SCALE 12.0f
#define NUM_TURTLES 10
#define MAX_LINES 12
#define MAX_TURN (5.0f * DEG2RAD)
#define MAX_SPEED 1.0f

typedef struct VM
{
    ArvissCpu cpu;
    ArvissMemory memory;
    bool isBlocked;
} VM;

typedef struct Turtle
{
    VM vm;

    bool isActive;
    Vector2 lastPosition;
    Vector2 position;
    float angle;
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

static void LoadCode(ArvissMemory* memory, const char* filename)
{
    printf("--- Loading %s\n", filename);
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("--- Failed to load %s\n", filename);
        return;
    }
    size_t count = fread(memory->mem, 1, sizeof(memory->mem), fp);
    printf("--- Read %zd bytes\n", count);
    fclose(fp);
}

static void InitTurtle(Turtle* turtle)
{
    turtle->vm.cpu = (ArvissCpu){.memory = MemInit(&turtle->vm.memory)};
    ArvissReset(&turtle->vm.cpu, 0);
    turtle->vm.isBlocked = false;

    const char* filename = "../../../../examples/turtles/arviss/bin/turtle.bin";
    LoadCode(&turtle->vm.memory, filename);

    turtle->isActive = true;

    Home(turtle);
}

static void Home(Turtle* turtle)
{
    turtle->lastPosition = Vector2Zero();
    turtle->position = Vector2Zero();
    turtle->angle = 0.0f;
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

        float angle = turtle->angle;
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
        turtle->angle += turn;
        turtle->angle = NormalizeAngle(turtle->angle);
        turtle->turnRemaining -= turn;
        turtle->vm.isBlocked = !IsFZero(turtle->turnRemaining);
    }
}

static void UpdateTurtleVM(Turtle* turtle)
{
    if (turtle->vm.isBlocked)
    {
        return;
    }

    ArvissResult result = ArvissRun(&turtle->vm.cpu, QUANTUM);
    bool isOk = !ArvissResultIsTrap(result);

    if (!isOk)
    {
        const ArvissTrap trap = ArvissResultAsTrap(result);

        // Check for a syscall.
        if (trap.mcause == trENVIRONMENT_CALL_FROM_M_MODE)
        {
            // The syscall number is in a7 (x17).
            const uint32_t syscall = turtle->vm.cpu.xreg[17];

            // Service the syscall.
            switch (syscall)
            {
            case SYSCALL_EXIT:
                // The exit code is in a0 (x10).
                turtle->isActive = false;
                isOk = true;
                break;
            case SYSCALL_HOME:
                Home(turtle);
                isOk = true;
                break;
            case SYSCALL_AHEAD:
                // The distance is in a0 (x10).
                float distance = *(float*)&turtle->vm.cpu.xreg[10];
                SetAhead(turtle, distance);
                isOk = true;
                break;
            case SYSCALL_TURN:
                // The angle is in a0 (x10).
                float angle = *(float*)&turtle->vm.cpu.xreg[10];
                SetTurn(turtle, angle);
                isOk = true;
                break;
            case SYSCALL_GOTO:
                // The coordinates are in a0 and a1 (x10 and x11).
                float x = *(float*)&turtle->vm.cpu.xreg[10];
                float y = *(float*)&turtle->vm.cpu.xreg[11];
                Goto(turtle, x, y);
                isOk = true;
                break;
            case SYSCALL_SET_PEN_STATE:
                // The pen state is in a0 (x10).
                SetPenState(turtle, turtle->vm.cpu.xreg[10] != 0);
                isOk = true;
                break;
            case SYSCALL_SET_VISIBILITY:
                // The visibility state is in a0 (x10).
                SetVisibility(turtle, turtle->vm.cpu.xreg[10] != 0);
                isOk = true;
                break;
            case SYSCALL_SET_PEN_COLOUR:
                // The colour's RGBA components are given in a0 (x10).
                Color colour = GetColor((int)turtle->vm.cpu.xreg[10]);
                SetPenColour(turtle, colour);
                isOk = true;
                break;
            default:
                break;
            }

            if (isOk)
            {
                // Do this so that we can return from the trap.
                turtle->vm.cpu.pc = turtle->vm.cpu.mepc; // Restore the program counter from the machine exception program counter.
                turtle->vm.cpu.pc += 4;                  // ...and increment it as normal.
            }
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
        DrawShape(turtleShape, turtle->position, turtle->angle * RAD2DEG, turtle->penColour);
    }
}

static void InitTurtles(Turtle* turtles, int numTurtles)
{
    for (int i = 0; i < numTurtles; i++)
    {
        InitTurtle(&turtles[i]);
        turtles[i].angle = (float)i * DEG2RAD * 360.0f / numTurtles;
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

    Turtle turtles[NUM_TURTLES];
    InitTurtles(turtles, NUM_TURTLES);

    while (!WindowShouldClose())
    {
        UpdateTurtleVMs(turtles, NUM_TURTLES);
        MoveTurtles(turtles, NUM_TURTLES);

        BeginDrawing();
        ClearBackground(DARKBLUE);

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

    return 0;
}
