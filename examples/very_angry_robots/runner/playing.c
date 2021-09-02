#include "components/door_components.h"
#include "components/dynamic_components.h"
#include "components/static_components.h"
#include "components/wall_components.h"
#include "entities.h"
#include "raylib.h"
#include "raymath.h"
#include "screens.h"
#include "systems/movement_system.h"
#include "systems/player_action_system.h"
#include "systems/robot_action_system.h"

#include <stdint.h>

#define HWALLS 5
#define VWALLS 3
#define WALL_SIZE 224
#define BORDER ((SCREEN_HEIGHT - WALL_SIZE * VWALLS) / 4)
#define TOP_BORDER (BORDER * 3)
#define LEFT_BORDER ((SCREEN_WIDTH - WALL_SIZE * HWALLS) / 2)
#define TLX LEFT_BORDER
#define TLY TOP_BORDER

#define MAX_LINES 1024
#define LINE_THICKNESS 2

#define DOOR_SIZE (WALL_SIZE - 80)
#define DOOR_THICKNESS 4

#define MAX_ENTITIES 256

typedef struct Line
{
    Vector2 start;
    Vector2 end;
    Color color;
} Line;

static Line lines[MAX_LINES];
static int numLines = 0;

void DrawWall(float x, float y, bool isVertical);
void DrawDoor(float x, float y, bool isVertical);
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

void EntityDrawDoors(void)
{
    for (int id = 0, numEntities = Entities.Count(); id < numEntities; id++)
    {
        if (Entities.Is(id, bmDoor | bmStatic | bmDrawable))
        {
            Vector2 position = StaticComponents.GetPosition(id);
            bool isVertical = DoorComponents.IsVertical(id);
            DrawDoor(position.x, position.y, isVertical);
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

int MakeDoor(float x, float y, bool isVertical)
{
    int id = Entities.Create();
    Entities.Set(id, bmStatic);
    Entities.Set(id, bmDrawable);
    Entities.Set(id, bmDoor);
    StaticComponents.Set(id, &(StaticComponent){.position = {x, y}});
    DoorComponents.Set(id, &(DoorComponent){.vertical = isVertical});
    return id;
}

int MakeDoorFromGrid(int gridX, int gridY, bool isVertical)
{
    const float x = TLX + (float)gridX * WALL_SIZE + ((isVertical) ? 0 : WALL_SIZE / 2);
    const float y = TLY + (float)gridY * WALL_SIZE + ((isVertical) ? WALL_SIZE / 2 : 0);
    return MakeDoor(x, y, isVertical);
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

    // Doors.
    MakeDoorFromGrid(2, 0, horizontal);
    MakeDoorFromGrid(2, 3, horizontal);
    MakeDoorFromGrid(0, 1, vertical);
    MakeDoorFromGrid(5, 1, vertical);
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

void DrawWall(float x, float y, bool isVertical)
{
    Vector2 extents = (isVertical) ? (Vector2){0, WALL_SIZE / 2} : (Vector2){WALL_SIZE / 2, 0};
    AddLine(x - extents.x, y - extents.y, x + extents.x, y + extents.y, BLUE);
}

void DrawDoor(float x, float y, bool isVertical)
{
    Vector2 extents = (isVertical) ? (Vector2){DOOR_THICKNESS / 2, DOOR_SIZE / 2} : (Vector2){DOOR_SIZE / 2, DOOR_THICKNESS / 2};
    AddLine(x - extents.x, y - extents.y, x + extents.x, y - extents.y, YELLOW);
    AddLine(x + extents.x, y - extents.y, x + extents.x, y + extents.y, YELLOW);
    AddLine(x + extents.x, y + extents.y, x - extents.x, y + extents.y, YELLOW);
    AddLine(x - extents.x, y + extents.y, x - extents.x, y - extents.y, YELLOW);

    if (isVertical)
    {
        AddLine(x, y - WALL_SIZE / 2, x, y - DOOR_SIZE / 2, BLUE);
        AddLine(x, y + DOOR_SIZE / 2, x, y + WALL_SIZE / 2, BLUE);
    }
    else
    {
        AddLine(x - WALL_SIZE / 2, y, x - DOOR_SIZE / 2, y, BLUE);
        AddLine(x + DOOR_SIZE / 2, y, x + WALL_SIZE / 2, y, BLUE);
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
    EntityDrawWalls();
    EntityDrawDoors();
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
