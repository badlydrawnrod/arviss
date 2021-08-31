#pragma once

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

typedef enum ScreenId
{
    MENU,
    PLAYING
} ScreenId;

void UpdateMenu(void);
void DrawMenu(double alpha);
void UpdatePlaying(void);
void DrawPlaying(double alpha);

void SwitchTo(ScreenId screenId);
