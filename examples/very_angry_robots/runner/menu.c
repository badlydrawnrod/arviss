#include "raylib.h"
#include "screens.h"

void UpdateMenu(void)
{
    if (IsKeyReleased(KEY_SPACE))
    {
        SwitchTo(PLAYING);
    }
}

void DrawMenu(double alpha)
{
    (void)alpha;
    ClearBackground(DARKBLUE);
    BeginDrawing();
    DrawFPS(4, SCREEN_HEIGHT - 20);
    EndDrawing();
}
