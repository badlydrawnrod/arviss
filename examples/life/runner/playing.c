#include "raylib.h"
#include "screens.h"

#define NUM_COLS 32
#define NUM_ROWS 32

#define TILE_SIZE 16

#define BOARD_WIDTH (TILE_SIZE * NUM_COLS)
#define BOARD_HEIGHT (TILE_SIZE * NUM_ROWS)

#define BOARD_LEFT ((SCREEN_WIDTH - BOARD_WIDTH) / 2)
#define BOARD_TOP ((SCREEN_HEIGHT - BOARD_HEIGHT) / 2)

static void DrawBoard(void)
{
    int startX = BOARD_LEFT;
    int startY = BOARD_TOP;
    int endX = BOARD_LEFT + BOARD_WIDTH;
    int endY = BOARD_TOP + BOARD_HEIGHT;
    DrawRectangle(startX, startY, BOARD_WIDTH, BOARD_HEIGHT, BEIGE);
    for (int row = 0; row <= NUM_ROWS; row++)
    {
        int y = startY + (row * TILE_SIZE);
        DrawLine(startX, y, endX, y, DARKBROWN);
    }
    for (int col = 0; col <= NUM_COLS; col++)
    {
        int x = startX + (col * TILE_SIZE);
        DrawLine(x, startY, x, endY, DARKBROWN);
    }
}

void EnterPlaying(void)
{
}

void UpdatePlaying(void)
{
}

void UpdateFramePlaying(double elapsed)
{
}

void DrawPlaying(double alpha)
{
    (void)alpha;

    ClearBackground(BROWN);
    BeginDrawing();
    DrawBoard();
    DrawRectangle(0, 0, SCREEN_WIDTH, 32, GOLD);
    DrawText("Playing", 4, 4, 20, BLACK);
    DrawFPS(4, SCREEN_HEIGHT - 20);
    EndDrawing();
}

void CheckTriggersPlaying(void)
{
    if (IsKeyPressed(KEY_ESCAPE))
    {
        SwitchTo(MENU);
    }
}
