#include "screens.h"

typedef void (*ScreenEnterFn)(void);
typedef void (*ScreenExitFn)(void);
typedef void (*ScreenUpdateFn)(void);
typedef void (*ScreenDrawFn)(double);

typedef struct ScreenVtbl
{
    ScreenEnterFn Enter;
    ScreenExitFn Exit;
    ScreenUpdateFn Update;
    ScreenDrawFn Draw;
} ScreenVtbl;

static ScreenVtbl screens[] = {{.Enter = EnterMenu, .Update = UpdateMenu, .Draw = DrawMenu},
                               {.Enter = EnterPlaying, .Update = UpdatePlaying, .Draw = DrawPlaying}};
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
 * Called by the main loop whenever drawing is required.
 * @param alpha a value from 0.0 to 1.0 indicating how much into the next frame we are. Useful for interpolation.
 */
void DrawScreen(double alpha)
{
    screen->Draw(alpha);
}
