#include "contoller.h"
#include "raylib.h"
#include "screens.h"

void EnterMenu(void)
{
}

void UpdateMenu(void)
{
}

void DrawMenu(double alpha)
{
    (void)alpha;
    ClearBackground(DARKBLUE);
    BeginDrawing();
    const bool haveGamepad = IsGamepadAvailable(0);
    const char* message = (haveGamepad) ? "Press space or gamepad (A)" : "Press space";
    const int textWidth = MeasureText(message, 20);
    DrawText(message, (SCREEN_WIDTH - textWidth) / 2, SCREEN_HEIGHT / 2, 20, RAYWHITE);
    DrawFPS(4, SCREEN_HEIGHT - 20);
    EndDrawing();
}

void CheckTriggersMenu(void)
{
    if (IsKeyPressed(KEY_SPACE))
    {
        SetController(ctKEYBOARD);
        SwitchTo(PLAYING);
    }
    else if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
    {
        SetController(ctGAMEPAD0);
        SwitchTo(PLAYING);
    }
}
