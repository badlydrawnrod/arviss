#include "raylib.h"
#include "screens.h"

static bool wasSpacePressed = false;

void EnterMenu(void)
{
    wasSpacePressed = IsKeyDown(KEY_SPACE);
}

void UpdateMenu(void)
{
    const bool isSpacePressed = IsKeyDown(KEY_SPACE);
    if (!wasSpacePressed && isSpacePressed)
    {
        SwitchTo(PLAYING);
    }
    wasSpacePressed = isSpacePressed;
}

void DrawMenu(double alpha)
{
    (void)alpha;
    ClearBackground(DARKBLUE);
    BeginDrawing();
    DrawText("Menu", 4, 4, 24, RAYWHITE);
    DrawFPS(4, SCREEN_HEIGHT - 20);
    EndDrawing();
}
