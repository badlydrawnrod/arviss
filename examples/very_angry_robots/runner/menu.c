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
    DrawText("Menu", 4, 4, 20, RAYWHITE);
    DrawFPS(4, SCREEN_HEIGHT - 20);
    EndDrawing();
}

void CheckTriggersMenu(void)
{
    if (IsKeyPressed(KEY_SPACE))
    {
        SwitchTo(PLAYING);
    }
}
