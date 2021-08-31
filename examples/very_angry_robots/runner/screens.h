#pragma once

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

typedef enum ScreenId
{
    MENU,
    PLAYING
} ScreenId;

void EnterMenu(void);
void UpdateMenu(void);
void DrawMenu(double alpha);
void CheckTriggersMenu(void);

void EnterPlaying(void);
void UpdatePlaying(void);
void DrawPlaying(double alpha);
void CheckTriggersPlaying(void);

void SwitchTo(ScreenId screenId);
