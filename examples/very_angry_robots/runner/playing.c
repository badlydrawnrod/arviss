#include "raylib.h"
#include "screens.h"

static bool wasQuitPressed = false;

void EnterPlaying(void)
{
    wasQuitPressed = IsKeyDown(KEY_SPACE);
}

void UpdatePlaying(void)
{
    bool isQuitPressed = IsKeyDown(KEY_SPACE);
    if (!wasQuitPressed && isQuitPressed)
    {
        SwitchTo(MENU);
    }
    wasQuitPressed = isQuitPressed;
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
