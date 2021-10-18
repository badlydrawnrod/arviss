#define PHYSICS_FPS 60.0
#define BDR_LOOP_FIXED_UPDATE_INTERVAL_SECONDS (1.0 / PHYSICS_FPS)
#define BDR_LOOP_FIXED_UPDATE UpdateScreen
#define BDR_LOOP_DRAW DrawScreen
#define BDR_LOOP_CHECK_TRIGGERS CheckTriggers

#define BDR_LOOP_IMPLEMENTATION

#include "game_loop.h"
#include "raylib.h"
#include "screens.h"

#define TARGET_FPS 60

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Very Angry Robots III");
    SetTargetFPS(TARGET_FPS);
    SetTraceLogLevel(LOG_DEBUG);
    SetExitKey(KEY_F4);

    SwitchTo(MENU);
    RunMainLoop();

    CloseWindow();

    return 0;
}
