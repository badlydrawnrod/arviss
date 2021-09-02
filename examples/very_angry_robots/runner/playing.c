#include "components/door_components.h"
#include "components/dynamic_components.h"
#include "components/static_components.h"
#include "components/wall_components.h"
#include "entities.h"
#include "raylib.h"
#include "screens.h"
#include "systems/drawing_system.h"
#include "systems/movement_system.h"
#include "systems/player_action_system.h"
#include "systems/robot_action_system.h"

#define HWALLS 5
#define VWALLS 3
#define WALL_SIZE 224
#define BORDER ((SCREEN_HEIGHT - WALL_SIZE * VWALLS) / 4)
#define TOP_BORDER (BORDER * 3)
#define LEFT_BORDER ((SCREEN_WIDTH - WALL_SIZE * HWALLS) / 2)
#define TLX LEFT_BORDER
#define TLY TOP_BORDER

void MakeRobot(float x, float y)
{
    int id = Entities.Create();
    Entities.Set(id, bmDynamic | bmDrawable | bmRobot | bmCollidable);
    DynamicComponents.Set(id, &(DynamicComponent){.position = {x, y}});
}

int MakePlayer(float x, float y)
{
    int id = Entities.Create();
    Entities.Set(id, bmDynamic | bmDrawable | bmPlayer | bmCollidable);
    DynamicComponents.Set(id, &(DynamicComponent){.position = {x, y}});
    return id;
}

int MakeWall(float x, float y, bool isVertical)
{
    int id = Entities.Create();
    Entities.Set(id, bmStatic | bmDrawable | bmWall | bmCollidable);
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
    Entities.Set(id, bmStatic | bmDrawable | bmDoor | bmCollidable);
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

void DrawPlaying(double alpha)
{
    (void)alpha;

    ClearBackground(BLACK);
    BeginDrawing();
    DrawText("Playing", 4, 4, 20, RAYWHITE);
    DrawingSystem.Update();
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
