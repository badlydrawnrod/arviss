#include "screens.h"

typedef void (*ScreenEnterFn)(void);
typedef void (*ScreenExitFn)(void);
typedef void (*ScreenUpdateFn)(void);
typedef void (*ScreenUpdateFrameFn)(double);
typedef void (*ScreenDrawFn)(double);
typedef void (*ScreenCheckTriggersFn)(void);

typedef struct ScreenVtbl
{
    ScreenEnterFn Enter;
    ScreenExitFn Exit;
    ScreenUpdateFn Update;
    ScreenUpdateFrameFn UpdateFrame;
    ScreenDrawFn Draw;
    ScreenCheckTriggersFn CheckTriggers;
} ScreenVtbl;

static ScreenVtbl screens[] = {{.Enter = EnterMenu,
                                .Update = UpdateMenu,
                                .UpdateFrame = UpdateFrameMenu,
                                .Draw = DrawMenu,
                                .CheckTriggers = CheckTriggersMenu},
                               {.Enter = EnterPlaying,
                                .Update = UpdatePlaying,
                                .UpdateFrame = UpdateFramePlaying,
                                .Draw = DrawPlaying,
                                .CheckTriggers = CheckTriggersPlaying}};
static ScreenId screenId = MENU;
static ScreenVtbl* screen = &screens[MENU];

void SwitchTo(ScreenId newScreenId)
{
    if (screenId != newScreenId)
    {
        if (screen->Exit)
        {
            screen->Exit();
        }
        screenId = newScreenId;
        screen = &screens[screenId];
        if (screen->Enter)
        {
            screen->Enter();
        }
    }
}

/**
 * Called by the main loop once per fixed timestep update interval.
 */
void UpdateScreen(void)
{
    screen->Update();
}

/**
 * Called by the main loop whenever a frame needs to be update (not drawn).
 */
void UpdateFrame(double elapsed)
{
    screen->UpdateFrame(elapsed);
}

/**
 * Called by the main loop whenever drawing is required.
 * @param alpha a value from 0.0 to 1.0 indicating how much into the next frame we are. Useful for interpolation.
 */
void DrawScreen(double alpha)
{
    screen->Draw(alpha);
}

/**
 * Called by the main loop when it's time to check edge-triggered events.
 */
void CheckTriggers(void)
{
    if (screen->CheckTriggers)
    {
        screen->CheckTriggers();
    }
}