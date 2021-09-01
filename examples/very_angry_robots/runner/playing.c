#include "contoller.h"
#include "entities.h"
#include "raylib.h"
#include "raymath.h"
#include "screens.h"

#include <stdint.h>

#define HWALLS 5
#define VWALLS 3
#define WALL_SIZE 224
#define BORDER ((SCREEN_HEIGHT - WALL_SIZE * VWALLS) / 4)
#define TOP_BORDER (BORDER * 3)
#define LEFT_BORDER ((SCREEN_WIDTH - WALL_SIZE * HWALLS) / 2)
#define TLX LEFT_BORDER
#define TLY TOP_BORDER
#define TRX (LEFT_BORDER + HWALLS * WALL_SIZE)
#define BRY (TOP_BORDER + VWALLS * WALL_SIZE)

#define MAX_LINES 1024
#define LINE_THICKNESS 2

#define DOOR_INDENT 40
#define DOOR_THICKNESS 4

#define PLAYER_SPEED 2

#define MAX_ENTITIES 256

typedef uint32_t Bitmap;

typedef struct StaticComponent
{
    Vector2 position;
} StaticComponent;

typedef struct DynamicComponent
{
    Vector2 position;
    Vector2 movement;
} DynamicComponent;

static struct DynamicComponent dynamicComponents[MAX_ENTITIES];

DynamicComponent* GetDynamicComponent(int id)
{
    return &dynamicComponents[id];
}

void SetDynamicComponent(int id, DynamicComponent* dynamicComponent)
{
    dynamicComponents[id] = *dynamicComponent;
}

Vector2 GetDynamicComponentPosition(int id)
{
    return dynamicComponents[id].position;
}

static struct
{
    DynamicComponent* (*Get)(int id);
    void (*Set)(int id, DynamicComponent* dynamicComponent);
    Vector2 (*GetPosition)(int id);
} DynamicComponents = {.Get = GetDynamicComponent, .Set = SetDynamicComponent, .GetPosition = GetDynamicComponentPosition};

typedef struct Wall
{
    Vector2 start;
    Vector2 end;
} Wall;

typedef struct Door
{
    Vector2 start;
    Vector2 end;
} Door;

typedef enum CommandType
{
    dcMOVE_TO,
    dcLINE_TO,
    dcCOLOR,
    dcSTOP
} CommandType;

typedef struct Command
{
    CommandType type;
    union
    {
        Vector2 target;
        Color color;
    };
} Command;

typedef struct Line
{
    Vector2 start;
    Vector2 end;
    Color color;
} Line;

static Wall walls[] = {
        {.start = {TLX, TLY}, .end = {TLX + 2 * WALL_SIZE, TLY}}, {.start = {TLX + 3 * WALL_SIZE, TLY}, .end = {TRX, TLY}},
        {.start = {TLX, BRY}, .end = {TLX + 2 * WALL_SIZE, BRY}}, {.start = {TLX + 3 * WALL_SIZE, BRY}, .end = {TRX, BRY}},
        {.start = {TLX, TLY}, .end = {TLX, TLY + WALL_SIZE}},     {.start = {TRX, TLY}, .end = {TRX, TLY + WALL_SIZE}},
        {.start = {TLX, TLY + 2 * WALL_SIZE}, .end = {TLX, BRY}}, {.start = {TRX, TLY + 2 * WALL_SIZE}, .end = {TRX, BRY}},
};

static Door doors[] = {
        {.start = {TLX + 2 * WALL_SIZE, TLY}, .end = {TLX + 3 * WALL_SIZE, TLY}},
        {.start = {TLX + 2 * WALL_SIZE, BRY}, .end = {TLX + 3 * WALL_SIZE, BRY}},
        {.start = {TLX, TLY + WALL_SIZE}, {TLX, TLY + 2 * WALL_SIZE}},
        {.start = {TRX, TLY + WALL_SIZE}, {TRX, TLY + 2 * WALL_SIZE}},
};

static Line lines[MAX_LINES];
static int numLines = 0;
static Vector2 playerPos;

void DrawPlayer(float x, float y);
void DrawRobot(float x, float y);

void EntityDrawPlayers(void)
{
    for (int id = 0, numEntities = Entities.Count(); id < numEntities; id++)
    {
        if (Entities.Is(id, bmPlayer | bmDynamic | bmDrawable))
        {
            Vector2 position = DynamicComponents.GetPosition(id);
            DrawPlayer(position.x, position.y);
        }
    }
}

void EntityDrawRobots(void)
{
    for (int id = 0, numEntities = Entities.Count(); id < numEntities; id++)
    {
        if (Entities.Is(id, bmRobot | bmDynamic | bmDrawable))
        {
            Vector2 position = DynamicComponents.GetPosition(id);
            DrawRobot(position.x, position.y);
        }
    }
}

void MakeRobot(float x, float y)
{
    int id = Entities.Create();
    Entities.Set(id, bmDynamic);
    Entities.Set(id, bmDrawable);
    Entities.Set(id, bmRobot);
    Entities.Set(id, bmCollidable);
    DynamicComponents.Set(id, &(DynamicComponent){.position = {x, y}});
}

void MakePlayer(float x, float y)
{
    int id = Entities.Create();
    Entities.Set(id, bmDynamic);
    Entities.Set(id, bmDrawable);
    Entities.Set(id, bmPlayer);
    Entities.Set(id, bmCollidable);
    DynamicComponents.Set(id, &(DynamicComponent){.position = {x, y}});
}

void EnterPlaying(void)
{
    playerPos = (Vector2){SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};
    Entities.Reset();
    MakePlayer(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    MakeRobot(SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2);
    MakeRobot(3 * SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2);
    MakeRobot(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 4);
    MakeRobot(SCREEN_WIDTH / 2, 3 * SCREEN_HEIGHT / 4);
}

void UpdatePlayer(void)
{
    // Find out what the player wants to do.
    float dx = 0.0f;
    float dy = 0.0f;
    const bool haveGamepad = IsGamepadAvailable(0);
    const bool usingKeyboard = GetController() == ctKEYBOARD || !haveGamepad;

    const bool isLeftDPadDown = haveGamepad && IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
    const bool isRightDPadDown = haveGamepad && IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
    const bool isUpDPadDown = haveGamepad && IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP);
    const bool isDownDPadDown = haveGamepad && IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
    const bool anyGamepadButtonDown = isLeftDPadDown || isRightDPadDown || isUpDPadDown || isDownDPadDown;

    if (usingKeyboard || anyGamepadButtonDown)
    {
        if (isRightDPadDown || IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        {
            dx += 1.0f;
        }
        if (isLeftDPadDown || IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        {
            dx -= 1.0f;
        }
        if (isUpDPadDown || IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        {
            dy -= 1.0f;
        }
        if (isDownDPadDown || IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
        {
            dy += 1.0f;
        }

        // Keep the speed the same when moving diagonally.
        if (dx != 0.0f && dy != 0.0f)
        {
            const float sqrt2 = 0.7071067811865475f;
            dx *= sqrt2;
            dy *= sqrt2;
        }
    }
    else
    {
        const float padX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        const float padY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        const float angle = atan2f(padY, padX);
        float distance = hypotf(padX, padY);
        if (distance > 1.0f)
        {
            distance = 1.0f;
        }
        dx = cosf(angle) * distance;
        dy = sinf(angle) * distance;
    }

    // Update its position in the world.
    playerPos.x += dx * PLAYER_SPEED;
    playerPos.y += dy * PLAYER_SPEED;
}

void UpdateRobots(void)
{
    // TODO: update the robots using Arviss.
}

void UpdatePlaying(void)
{
    UpdatePlayer();
    UpdateRobots();
}

void BeginDrawLines(void)
{
    numLines = 0;
}

void EndDrawLines(void)
{
    for (int i = 0; i < numLines; i++)
    {
        DrawLineEx(lines[i].start, lines[i].end, LINE_THICKNESS, lines[i].color);
    }
}

void AddLine(float startX, float startY, float endX, float endY, Color color)
{
    lines[numLines].start.x = startX;
    lines[numLines].start.y = startY;
    lines[numLines].end.x = endX;
    lines[numLines].end.y = endY;
    lines[numLines].color = color;
    ++numLines;
}

void AddLineV(Vector2 start, Vector2 end, Color color)
{
    lines[numLines].start = start;
    lines[numLines].end = end;
    lines[numLines].color = color;
    ++numLines;
}

void DrawWalls(void)
{
    for (int i = 0; i < sizeof(walls) / sizeof(walls[0]); i++)
    {
        AddLineV(walls[i].start, walls[i].end, BLUE);
    }
}

void DrawDoors(void)
{
    for (int i = 0; i < sizeof(doors) / sizeof(doors[0]); i++)
    {
        const Door* door = &doors[i];
        const bool isHorizontal = (int)door->start.y == (int)door->end.y;
        const float startX = door->start.x;
        const float startY = door->start.y;
        const float endX = door->end.x;
        const float endY = door->end.y;
        if (isHorizontal)
        {
            AddLine(startX, startY, startX + DOOR_INDENT, startY, BLUE);
            AddLine(endX - DOOR_INDENT, endY, endX, endY, BLUE);

            AddLine(startX + DOOR_INDENT, startY - DOOR_THICKNESS, endX - DOOR_INDENT, endY - DOOR_THICKNESS, YELLOW);
            AddLine(startX + DOOR_INDENT, startY + DOOR_THICKNESS, endX - DOOR_INDENT, endY + DOOR_THICKNESS, YELLOW);
            AddLine(startX + DOOR_INDENT, startY - DOOR_THICKNESS, startX + DOOR_INDENT, startY + DOOR_THICKNESS, YELLOW);
            AddLine(endX - DOOR_INDENT, endY - DOOR_THICKNESS, endX - DOOR_INDENT, endY + DOOR_THICKNESS, YELLOW);
        }
        else
        {
            AddLine(startX, startY, startX, startY + DOOR_INDENT, BLUE);
            AddLine(endX, endY - DOOR_INDENT, endX, endY, BLUE);

            AddLine(startX - DOOR_THICKNESS, startY + DOOR_INDENT, endX - DOOR_THICKNESS, endY - DOOR_INDENT, YELLOW);
            AddLine(startX + DOOR_THICKNESS, startY + DOOR_INDENT, endX + DOOR_THICKNESS, endY - DOOR_INDENT, YELLOW);
            AddLine(startX - DOOR_THICKNESS, startY + DOOR_INDENT, startX + DOOR_THICKNESS, startY + DOOR_INDENT, YELLOW);
            AddLine(endX - DOOR_THICKNESS, endY - DOOR_INDENT, endX + DOOR_THICKNESS, endY - DOOR_INDENT, YELLOW);
        }
    }
}

void DrawRobot(float x, float y)
{
    // Upper body.
    AddLine(x, y, x - 16, y, LIME);
    AddLine(x - 16, y, x - 8, y - 16, LIME);
    AddLine(x - 8, y - 16, x + 8, y - 16, LIME);
    AddLine(x + 8, y - 16, x + 16, y, LIME);
    AddLine(x + 16, y, x, y, LIME);

    // Eyes.
    AddLine(x - 8, y - 6, x - 4, y - 6, RED);
    AddLine(x, y - 6, x + 4, y - 6, RED);

    // Lower body.
    AddLine(x - 12, y + 2, x + 12, y + 2, DARKGREEN);
    AddLine(x - 8, y + 4, x + 8, y + 4, DARKGREEN);
}

void DrawPlayer(float x, float y)
{
    Color bodyColour = SKYBLUE;
    Color headColour = SKYBLUE;
    Color legColour = SKYBLUE;

    // Body.
    AddLine(x, y, x, y - 12, bodyColour);

    // Arms.
    AddLine(x, y - 12, x - 8, y, bodyColour);
    AddLine(x, y - 12, x + 8, y, bodyColour);

    // Legs.
    AddLine(x, y, x - 8, y + 16, legColour);
    AddLine(x, y, x + 8, y + 16, legColour);

    // Head.
    AddLine(x, y - 12, x - 6, y - 16, headColour);
    AddLine(x, y - 12, x + 6, y - 16, headColour);
    AddLine(x - 6, y - 16, x, y - 20, headColour);
    AddLine(x + 6, y - 16, x, y - 20, headColour);
}

void DrawPlaying(double alpha)
{
    (void)alpha;

    ClearBackground(BLACK);
    BeginDrawing();
    DrawText("Playing", 4, 4, 20, RAYWHITE);
    BeginDrawLines();
    DrawWalls();
    DrawDoors();
    EntityDrawRobots();
    EntityDrawPlayers();
    EndDrawLines();
    DrawFPS(4, SCREEN_HEIGHT - 20);
    EndDrawing();
}

void CheckTriggersPlaying(void)
{
    if (IsKeyPressed(KEY_SPACE))
    {
        SwitchTo(MENU);
    }
}
