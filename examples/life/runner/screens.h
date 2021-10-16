#pragma once

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 1024

typedef enum ScreenId
{
    MENU,
    PLAYING
} ScreenId;

void EnterMenu(void);
void UpdateMenu(void);
void UpdateFrameMenu(double elapsed);
void DrawMenu(double alpha);
void CheckTriggersMenu(void);

void EnterPlaying(void);
void UpdatePlaying(void);
void UpdateFramePlaying(double elapsed);
void DrawPlaying(double alpha);
void CheckTriggersPlaying(void);

void SwitchTo(ScreenId screenId);
