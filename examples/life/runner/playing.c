#include "raylib.h"
#include "screens.h"

#include <stdbool.h>

#define NUM_COLS 40
#define NUM_ROWS 40

#define TILE_SIZE 16

#define BOARD_WIDTH (TILE_SIZE * NUM_COLS)
#define BOARD_HEIGHT (TILE_SIZE * NUM_ROWS)

#define BOARD_LEFT ((SCREEN_WIDTH - BOARD_WIDTH) / 2)
#define BOARD_TOP ((SCREEN_HEIGHT - BOARD_HEIGHT) / 2)

/*
 * From: https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life
 * - Any live cell with two or three live neighbours survives.
 * - Any dead cell with three live neighbours becomes a live cell.
 * - All other live cells die in the next generation. Similarly, all other dead cells stay dead.
 */

static bool board[2][NUM_ROWS * NUM_COLS];
static int current = 0;
static int next = 1;

static inline void SetNext(int row, int col, bool value)
{
    board[next][row * NUM_COLS + col] = value;
}

static inline void SetCurrent(int row, int col, bool value)
{
    board[current][row * NUM_COLS + col] = value;
}

static inline bool GetCurrent(int row, int col)
{
    return board[current][row * NUM_COLS + col];
}

static void PopulateBoard(void)
{
    for (int row = 0; row < NUM_ROWS; row++)
    {
        for (int col = 0; col < NUM_COLS; col++)
        {
            SetCurrent(row, col, GetRandomValue(0, 999) < 250);
        }
    }
}

static int CountNeighbours(int row, int col)
{
    int total = 0;
    for (int y = row - 1; y < row + 2; y++)
    {
        for (int x = col - 1; x < col + 2; x++)
        {
            if ((x != col) || (y != row))
            {
                if (y >= 0 && x >= 0 && y < NUM_ROWS && x < NUM_COLS)
                {
                    if (GetCurrent(y, x))
                    {
                        ++total;
                    }
                }
            }
        }
    }
    return total;
}

static void UpdateBoard(void)
{
    for (int row = 0; row < NUM_ROWS; row++)
    {
        for (int col = 0; col < NUM_COLS; col++)
        {
            bool wasAlive = GetCurrent(row, col);
            int neighbours = CountNeighbours(row, col);
            SetNext(row, col, wasAlive && neighbours == 2 || neighbours == 3);
        }
    }
    int t = current;
    current = next;
    next = t;
}

static void DrawBoard(void)
{
    int startX = BOARD_LEFT;
    int startY = BOARD_TOP;
    int endX = BOARD_LEFT + BOARD_WIDTH;
    int endY = BOARD_TOP + BOARD_HEIGHT;

    // Draw the background.
    DrawRectangle(startX, startY, BOARD_WIDTH, BOARD_HEIGHT, BEIGE);

    // Draw the content.
    for (int row = 0; row < NUM_ROWS; row++)
    {
        int y = startY + (row * TILE_SIZE);
        for (int col = 0; col < NUM_COLS; col++)
        {
            int x = startX + (col * TILE_SIZE);
            if (GetCurrent(row, col))
            {
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, LIME);
            }
        }
    }

    // Draw the grid.
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
    PopulateBoard();
}

void UpdatePlaying(void)
{
    UpdateBoard();
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
