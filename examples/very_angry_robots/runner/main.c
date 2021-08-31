#define PHYSICS_FPS 50.0
#define BDR_LOOP_FIXED_UPDATE_INTERVAL_SECONDS (1.0 / PHYSICS_FPS)
#define BDR_LOOP_FIXED_UPDATE Update
#define BDR_LOOP_DRAW Draw
#define BDR_LOOP_IMPLEMENTATION

#include "game_loop.h"
#include "raylib.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define TARGET_FPS 60

/**
 * Called by the main loop once per fixed timestep update interval.
 */
void Update(void)
{
}

/**
 * Called by the main loop whenever drawing is required.
 * @param alpha a value from 0.0 to 1.0 indicating how much into the next frame we are. Useful for interpolation.
 */
void Draw(double alpha)
{
    ClearBackground(BLACK);
    BeginDrawing();
    DrawFPS(4, SCREEN_HEIGHT - 20);
    EndDrawing();
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Very Angry Robots III");
    SetTargetFPS(TARGET_FPS);

    RunMainLoop();

    CloseWindow();

    return 0;
}
