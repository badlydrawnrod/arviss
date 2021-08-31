#include "screens.h"

typedef void (*ScreenUpdateFn)(void);
typedef void (*ScreenDrawFn)(double);

typedef struct ScreenVtbl
{
    ScreenUpdateFn Update;
    ScreenDrawFn Draw;
} ScreenVtbl;

static ScreenVtbl screens[] = {{.Update = UpdateMenu, .Draw = DrawMenu}, {.Update = UpdatePlaying, .Draw = DrawPlaying}};
static ScreenId screenId = MENU;
static ScreenVtbl* screen = &screens[MENU];

void SwitchTo(ScreenId newScreenId)
{
    if (screenId != newScreenId)
    {
        screenId = newScreenId;
        screen = &screens[screenId];
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
