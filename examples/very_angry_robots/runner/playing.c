#include "components/collidable_components.h"
#include "components/door_components.h"
#include "components/positions.h"
#include "components/velocities.h"
#include "components/wall_components.h"
#include "entities.h"
#include "raylib.h"
#include "screens.h"
#include "systems/collision_system.h"
#include "systems/drawing_system.h"
#include "systems/event_system.h"
#include "systems/movement_system.h"
#include "systems/player_action_system.h"
#include "systems/reaper_system.h"
#include "systems/robot_action_system.h"

#define HWALLS 5
#define VWALLS 3
#define WALL_SIZE 224
#define BORDER ((SCREEN_HEIGHT - WALL_SIZE * VWALLS) / 4)
#define TOP_BORDER (BORDER * 3)
#define LEFT_BORDER ((SCREEN_WIDTH - WALL_SIZE * HWALLS) / 2)
#define TLX LEFT_BORDER
#define TLY TOP_BORDER

EntityId MakeRobot(float x, float y)
{
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmVelocity | bmDrawable | bmRobot | bmCollidable);
    Positions.Set(id, &(Position){.position = {x, y}});
    Velocities.Set(id, &(Velocity){.velocity = {1.0f, 0.0f}});
    CollidableComponents.Set(id, &(CollidableComponent){.type = ctROBOT});
    return id;
}

EntityId MakePlayer(float x, float y)
{
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmVelocity | bmDrawable | bmPlayer | bmCollidable);
    Positions.Set(id, &(Position){.position = {x, y}});
    Velocities.Set(id, &(Velocity){.velocity = {0.0f, 0.0f}});
    CollidableComponents.Set(id, &(CollidableComponent){.type = ctPLAYER});
    return id;
}

EntityId MakeWall(float x, float y, bool isVertical)
{
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmDrawable | bmWall | bmCollidable);
    Positions.Set(id, &(Position){.position = {x, y}});
    WallComponents.Set(id, &(WallComponent){.vertical = isVertical});
    CollidableComponents.Set(id, &(CollidableComponent){.type = isVertical ? ctVWALL : ctHWALL});
    return id;
}

EntityId MakeWallFromGrid(int gridX, int gridY, bool isVertical)
{
    const float x = TLX + (float)gridX * WALL_SIZE + ((isVertical) ? 0 : WALL_SIZE / 2);
    const float y = TLY + (float)gridY * WALL_SIZE + ((isVertical) ? WALL_SIZE / 2 : 0);
    return MakeWall(x, y, isVertical);
}

EntityId MakeDoor(float x, float y, bool isVertical)
{
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmDrawable | bmDoor | bmCollidable);
    Positions.Set(id, &(Position){.position = {x, y}});
    DoorComponents.Set(id, &(DoorComponent){.vertical = isVertical});
    CollidableComponents.Set(id, &(CollidableComponent){.type = isVertical ? ctVDOOR : ctHDOOR});
    return id;
}

EntityId MakeDoorFromGrid(int gridX, int gridY, bool isVertical)
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
    ReaperSystem.Update();

    PlayerActionSystem.Update();
    RobotActionSystem.Update();
    MovementSystem.Update();
    CollisionSystem.Update();

    EventSystem.Update(); // TODO: possibly put this *before* the other systems.
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
