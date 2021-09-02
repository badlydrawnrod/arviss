#include "dynamic_components.h"
#include "entities.h"
#include "movement_system.h"
#include "player_action_system.h"
#include "raylib.h"
#include "raymath.h"
#include "robot_action_system.h"
#include "screens.h"
#include "static_components.h"
#include "wall_components.h"

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

#define MAX_ENTITIES 256

typedef struct Door
{
    Vector2 start;
    Vector2 end;
} Door;

typedef struct Line
{
    Vector2 start;
    Vector2 end;
    Color color;
} Line;

static Door doors[] = {
        {.start = {TLX + 2 * WALL_SIZE, TLY}, .end = {TLX + 3 * WALL_SIZE, TLY}},
        {.start = {TLX + 2 * WALL_SIZE, BRY}, .end = {TLX + 3 * WALL_SIZE, BRY}},
        {.start = {TLX, TLY + WALL_SIZE}, {TLX, TLY + 2 * WALL_SIZE}},
        {.start = {TRX, TLY + WALL_SIZE}, {TRX, TLY + 2 * WALL_SIZE}},
};

static Line lines[MAX_LINES];
static int numLines = 0;

void DrawWall(float x, float y, bool isVertical);
void DrawPlayer(float x, float y);
void DrawRobot(float x, float y);

void EntityDrawWalls(void)
{
    for (int id = 0, numEntities = Entities.Count(); id < numEntities; id++)
    {
        if (Entities.Is(id, bmWall | bmStatic | bmDrawable))
        {
            Vector2 position = StaticComponents.GetPosition(id);
            bool isVertical = WallComponents.IsVertical(id);
            DrawWall(position.x, position.y, isVertical);
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

void MakeRobot(float x, float y)
{
    int id = Entities.Create();
    Entities.Set(id, bmDynamic);
    Entities.Set(id, bmDrawable);
    Entities.Set(id, bmRobot);
    Entities.Set(id, bmCollidable);
    DynamicComponents.Set(id, &(DynamicComponent){.position = {x, y}});
}

int MakePlayer(float x, float y)
{
    int id = Entities.Create();
    Entities.Set(id, bmDynamic);
    Entities.Set(id, bmDrawable);
    Entities.Set(id, bmPlayer);
    Entities.Set(id, bmCollidable);
    DynamicComponents.Set(id, &(DynamicComponent){.position = {x, y}});
    return id;
}

int MakeWall(float x, float y, bool isVertical)
{
    int id = Entities.Create();
    Entities.Set(id, bmStatic);
    Entities.Set(id, bmDrawable);
    Entities.Set(id, bmWall);
    StaticComponents.Set(id, &(StaticComponent){.position = {x, y}});
    WallComponents.Set(id, &(WallComponent){.vertical = isVertical});
    return id;
}

int MakeWallFromGrid(int gridX, int gridY, bool isVertical)
{
    const float x = TLX + (float)gridX * WALL_SIZE + ((isVertical) ? 0 : WALL_SIZE / 2);
    const float y = TLY + (float)gridY * WALL_SIZE + ((isVertical) ? WALL_SIZE / 2 : 0);
    return MakeWall(x, y, isVertical);
}

void EnterPlaying(void)
{
    Entities.Reset();
    MakePlayer(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    MakeRobot(SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2);
    MakeRobot(3 * SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2);
    MakeRobot(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 4);
    MakeRobot(SCREEN_WIDTH / 2, 3 * SCREEN_HEIGHT / 4);

    const bool horizontal = false;
    const bool vertical = true;

    // Top walls.
    MakeWallFromGrid(0, 0, horizontal);
    MakeWallFromGrid(1, 0, horizontal);
    MakeWallFromGrid(3, 0, horizontal);
    MakeWallFromGrid(4, 0, horizontal);

    // Bottom walls.
    MakeWallFromGrid(0, 3, horizontal);
    MakeWallFromGrid(1, 3, horizontal);
    MakeWallFromGrid(3, 3, horizontal);
    MakeWallFromGrid(4, 3, horizontal);

    // Left walls.
    MakeWallFromGrid(0, 0, vertical);
    MakeWallFromGrid(0, 2, vertical);

    // Right walls.
    MakeWallFromGrid(5, 0, vertical);
    MakeWallFromGrid(5, 2, vertical);
}

void UpdatePlaying(void)
{
    PlayerActionSystem.Update();
    RobotActionSystem.Update();
    MovementSystem.Update();
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

void DrawWall(float x, float y, bool isVertical)
{
    Vector2 extents = (isVertical) ? (Vector2){0, WALL_SIZE / 2} : (Vector2){WALL_SIZE / 2, 0};
    AddLine(x - extents.x, y - extents.y, x + extents.x, y + extents.y, BLUE);
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
    EntityDrawWalls();
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
