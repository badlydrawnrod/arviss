#include "raylib.h"
#include "screens.h"

void UpdatePlaying(void)
{
    if (IsKeyReleased(KEY_Q))
    {
        SwitchTo(MENU);
    }
}

void DrawPlaying(double alpha)
{
    (void)alpha;
    ClearBackground(DARKBROWN);
    BeginDrawing();
    DrawFPS(4, SCREEN_HEIGHT - 20);
    EndDrawing();
}
