#include "raylib.h"
#include "screens.h"
#include "systems/collision_response_system.h"
#include "systems/collision_system.h"
#include "systems/drawing_system.h"
#include "systems/event_system.h"
#include "systems/game_status_system.h"
#include "systems/movement_system.h"
#include "systems/player_action_system.h"
#include "systems/reaper_system.h"
#include "systems/robot_action_system.h"

void EnterPlaying(void)
{
    Entities.Reset();
    EventSystem.Reset();
    CollisionResponseSystem.Reset();
    GameStatusSystem.Reset();
}

void UpdatePlaying(void)
{
    ReaperSystem.Update();

    PlayerActionSystem.Update();
    RobotActionSystem.Update();
    MovementSystem.Update();
    CollisionSystem.Update();
    CollisionResponseSystem.Update();
    GameStatusSystem.Update();

    EventSystem.Update();

    if (GameStatusSystem.IsGameOver())
    {
        SwitchTo(MENU);
    }
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
    if (IsKeyPressed(KEY_ESCAPE))
    {
        SwitchTo(MENU);
    }
}
