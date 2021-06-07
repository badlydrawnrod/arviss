#include "raylib.h"

#define BORDER_WIDTH 8

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Turtles");
    SetTargetFPS(60);

    RenderTexture2D target = LoadRenderTexture(screenWidth - (BORDER_WIDTH * 2), screenHeight - (BORDER_WIDTH * 2));
    BeginTextureMode(target);
    ClearBackground(BLACK);
    EndTextureMode();
    int x = (screenWidth - target.texture.width) / 2;
    int y = (screenHeight - target.texture.height) / 2;

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(DARKBLUE);
        BeginTextureMode(target);
        EndTextureMode();
        DrawTexture(target.texture, x, y, WHITE);
        EndDrawing();
    }
    CloseWindow();

    return 0;
}
