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

#define DOOR_INDENT 40
#define DOOR_THICKNESS 4

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
        const float startX = door->start.x;
        const float startY = door->start.y;
        const float endX = door->end.x;
        const float endY = door->end.y;
        if (isHorizontal)
        {
            AddLine((Vector2){startX, startY}, (Vector2){startX + DOOR_INDENT, startY}, BLUE);
            AddLine((Vector2){endX - DOOR_INDENT, endY}, (Vector2){endX, endY}, BLUE);

            AddLine((Vector2){startX + DOOR_INDENT, startY - DOOR_THICKNESS}, (Vector2){endX - DOOR_INDENT, endY - DOOR_THICKNESS},
                    YELLOW);
            AddLine((Vector2){startX + DOOR_INDENT, startY + DOOR_THICKNESS}, (Vector2){endX - DOOR_INDENT, endY + DOOR_THICKNESS},
                    YELLOW);
            AddLine((Vector2){startX + DOOR_INDENT, startY - DOOR_THICKNESS},
                    (Vector2){startX + DOOR_INDENT, startY + DOOR_THICKNESS}, YELLOW);
            AddLine((Vector2){endX - DOOR_INDENT, endY - DOOR_THICKNESS}, (Vector2){endX - DOOR_INDENT, endY + DOOR_THICKNESS},
                    YELLOW);
        }
        else
        {
            AddLine((Vector2){startX, startY}, (Vector2){startX, startY + DOOR_INDENT}, BLUE);
            AddLine((Vector2){endX, endY - DOOR_INDENT}, (Vector2){endX, endY}, BLUE);

            AddLine((Vector2){startX - DOOR_THICKNESS, startY + DOOR_INDENT}, (Vector2){endX - DOOR_THICKNESS, endY - DOOR_INDENT},
                    YELLOW);
            AddLine((Vector2){startX + DOOR_THICKNESS, startY + DOOR_INDENT}, (Vector2){endX + DOOR_THICKNESS, endY - DOOR_INDENT},
                    YELLOW);
            AddLine((Vector2){startX - DOOR_THICKNESS, startY + DOOR_INDENT},
                    (Vector2){startX + DOOR_THICKNESS, startY + DOOR_INDENT}, YELLOW);
            AddLine((Vector2){endX - DOOR_THICKNESS, endY - DOOR_INDENT}, (Vector2){endX + DOOR_THICKNESS, endY - DOOR_INDENT},
                    YELLOW);
        }
    }
}

void DrawRobot(float x, float y)
{
    AddLine((Vector2){x, y}, (Vector2){x - 16, y}, LIME);
    AddLine((Vector2){x - 16, y}, (Vector2){x - 8, y - 16}, LIME);
    AddLine((Vector2){x - 8, y - 16}, (Vector2){x + 8, y - 16}, LIME);
    AddLine((Vector2){x + 8, y - 16}, (Vector2){x + 16, y}, LIME);
    AddLine((Vector2){x + 16, y}, (Vector2){x, y}, LIME);
    AddLine((Vector2){x - 8, y - 6}, (Vector2){x - 4, y - 6}, RED);
    AddLine((Vector2){x, y - 6}, (Vector2){x + 4, y - 6}, RED);
}

void DrawRobots(void)
{
    DrawRobot(SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2);
    DrawRobot(3 * SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2);
    DrawRobot(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 4);
    DrawRobot(SCREEN_WIDTH / 2, 3 * SCREEN_HEIGHT / 4);
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
    DrawRobots();
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
