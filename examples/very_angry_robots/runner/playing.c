#include "raylib.h"
#include "screens.h"

#define HWALLS 5
#define VWALLS 3
#define WALL_SIZE 224
#define BORDER ((SCREEN_HEIGHT - WALL_SIZE * VWALLS) / 4)
#define TOP_BORDER (BORDER * 3)
#define LEFT_BORDER ((SCREEN_WIDTH - WALL_SIZE * HWALLS) / 2)
#define TLX LEFT_BORDER
#define TLY TOP_BORDER
#define TRX (LEFT_BORDER + HWALLS * WALL_SIZE)
#define BRY (TOP_BORDER + VWALLS * WALL_SIZE)

#define MAX_LINES 1024
#define LINE_THICKNESS 2

typedef struct Wall
{
    Vector2 start;
    Vector2 end;
} Wall;

typedef struct Door
{
    Vector2 start;
    Vector2 end;
} Door;

typedef enum CommandType
{
    dcMOVE_TO,
    dcLINE_TO,
    dcCOLOR,
    dcSTOP
} CommandType;

typedef struct Command
{
    CommandType type;
    union
    {
        Vector2 target;
        Color color;
    };
} Command;

typedef struct Line
{
    Vector2 start;
    Vector2 end;
    Color color;
} Line;

static Wall walls[] = {
        {.start = {TLX, TLY}, .end = {TLX + 2 * WALL_SIZE, TLY}}, {.start = {TLX + 3 * WALL_SIZE, TLY}, .end = {TRX, TLY}},
        {.start = {TLX, BRY}, .end = {TLX + 2 * WALL_SIZE, BRY}}, {.start = {TLX + 3 * WALL_SIZE, BRY}, .end = {TRX, BRY}},
        {.start = {TLX, TLY}, .end = {TLX, TLY + WALL_SIZE}},     {.start = {TRX, TLY}, .end = {TRX, TLY + WALL_SIZE}},
        {.start = {TLX, TLY + 2 * WALL_SIZE}, .end = {TLX, BRY}}, {.start = {TRX, TLY + 2 * WALL_SIZE}, .end = {TRX, BRY}},
};

static Door doors[] = {
        {.start = {TLX + 2 * WALL_SIZE, TLY}, .end = {TLX + 3 * WALL_SIZE, TLY}},
        {.start = {TLX + 2 * WALL_SIZE, BRY}, .end = {TLX + 3 * WALL_SIZE, BRY}},
        {.start = {TLX, TLY + WALL_SIZE}, {TLX, TLY + 2 * WALL_SIZE}},
        {.start = {TRX, TLY + WALL_SIZE}, {TRX, TLY + 2 * WALL_SIZE}},
};

static Line lines[MAX_LINES];
static int numLines = 0;

void EnterPlaying(void)
{
}

void UpdatePlaying(void)
{
}

void BeginDrawLines(void)
{
    numLines = 0;
}

void EndDrawLines(void)
{
    for (int i = 0; i < numLines; i++)
    {
        DrawLineEx(lines[i].start, lines[i].end, LINE_THICKNESS, lines[i].color);
    }
}

void AddLine(Vector2 start, Vector2 end, Color color)
{
    lines[numLines].start = start;
    lines[numLines].end = end;
    lines[numLines].color = color;
    ++numLines;
}

void DrawWalls(void)
{
    for (int i = 0; i < sizeof(walls) / sizeof(walls[0]); i++)
    {
        AddLine(walls[i].start, walls[i].end, BLUE);
    }
}

void DrawDoors(void)
{
    for (int i = 0; i < sizeof(doors) / sizeof(doors[0]); i++)
    {
        const Door* door = &doors[i];

        const bool isHorizontal = (int)door->start.y == (int)door->end.y;
        if (isHorizontal)
        {
            AddLine((Vector2){door->start.x, door->start.y - 4}, (Vector2){door->end.x, door->end.y - 4}, YELLOW);
            AddLine((Vector2){door->start.x, door->start.y + 4}, (Vector2){door->end.x, door->end.y + 4}, YELLOW);
            AddLine((Vector2){door->start.x, door->start.y - 4}, (Vector2){door->start.x, door->start.y + 4}, YELLOW);
            AddLine((Vector2){door->end.x, door->end.y - 4}, (Vector2){door->end.x, door->end.y + 4}, YELLOW);
        }
        else
        {
            AddLine((Vector2){door->start.x - 4, door->start.y}, (Vector2){door->end.x - 4, door->end.y}, YELLOW);
            AddLine((Vector2){door->start.x + 4, door->start.y}, (Vector2){door->end.x + 4, door->end.y}, YELLOW);
            AddLine((Vector2){door->start.x - 4, door->start.y}, (Vector2){door->start.x + 4, door->start.y}, YELLOW);
            AddLine((Vector2){door->end.x - 4, door->end.y}, (Vector2){door->end.x + 4, door->end.y}, YELLOW);
        }
    }
}

void DrawPlaying(double alpha)
{
    (void)alpha;

    ClearBackground(BLACK);
    BeginDrawing();
    DrawText("Playing", 4, 4, 20, RAYWHITE);
    BeginDrawLines();
    DrawWalls();
    DrawDoors();
    EndDrawLines();
    DrawFPS(4, SCREEN_HEIGHT - 20);
    EndDrawing();
}

void CheckTriggersPlaying(void)
{
    if (IsKeyPressed(KEY_SPACE))
    {
        SwitchTo(MENU);
    }
}
