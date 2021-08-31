#include "raylib.h"
#include "screens.h"

void EnterPlaying(void)
{
}

void UpdatePlaying(void)
{
}

void DrawPlaying(double alpha)
{
    (void)alpha;

    ClearBackground(DARKBROWN);
    BeginDrawing();
    DrawText("Playing", 4, 4, 24, RAYWHITE);
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
