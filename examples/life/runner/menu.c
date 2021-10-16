#include "raylib.h"
#include "screens.h"

void EnterMenu(void)
{
}

void UpdateMenu(void)
{
}

void UpdateFrameMenu(double elapsed)
{
}

void DrawMenu(double alpha)
{
    (void)alpha;
    ClearBackground(BROWN);
    BeginDrawing();
    DrawRectangle(0, 0, SCREEN_WIDTH, 32, GOLD);
    DrawText("Menu", 4, 4, 20, BLACK);
    const char* message = "Press space";
    const int textWidth = MeasureText(message, 20);
    DrawRectangle((SCREEN_WIDTH - textWidth) / 2 - 8, 3 * SCREEN_HEIGHT / 4 - 2, textWidth + 16, 24, BEIGE);
    DrawText(message, (SCREEN_WIDTH - textWidth) / 2, 3 * SCREEN_HEIGHT / 4, 20, BLACK);
    DrawFPS(4, SCREEN_HEIGHT - 20);
    EndDrawing();
}

void CheckTriggersMenu(void)
{
    if (IsKeyPressed(KEY_SPACE))
    {
        SwitchTo(scrPLAYING);
    }
}
