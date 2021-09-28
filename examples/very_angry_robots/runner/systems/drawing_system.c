#include "drawing_system.h"

#include "entities.h"
#include "raylib.h"
#include "systems/event_system.h"
#include "tables/aims.h"
#include "tables/doors.h"
#include "tables/events.h"
#include "tables/player_status.h"
#include "tables/positions.h"
#include "tables/rooms.h"
#include "tables/velocities.h"
#include "tables/walls.h"
#include "types.h"

#define MAX_LINES 1024
#define LINE_THICKNESS 2

#define WALL_SIZE 224
#define DOOR_SIZE (WALL_SIZE - 80)
#define DOOR_THICKNESS 4
#define HWALLS 5
#define VWALLS 3

typedef struct Line
{
    Vector2 start;
    Vector2 end;
    Color color;
} Line;

static Line lines[MAX_LINES];
static int numLines = 0;
static RoomId nextRoom;
static RoomId currentRoom;
static Vector2 secondCamera;
static GameTime exitTime;

static void BeginDrawLines(void)
{
    numLines = 0;
}

static void EndDrawLines(void)
{
    for (int i = 0; i < numLines; i++)
    {
        DrawLineEx(lines[i].start, lines[i].end, LINE_THICKNESS, lines[i].color);
    }
}

static void AddLine(float startX, float startY, float endX, float endY, Color color)
{
    lines[numLines].start.x = startX;
    lines[numLines].start.y = startY;
    lines[numLines].end.x = endX;
    lines[numLines].end.y = endY;
    lines[numLines].color = color;
    ++numLines;
}

static inline void AddLineV(Vector2 start, Vector2 end, Color color)
{
    AddLine(start.x, start.y, end.x, end.y, color);
}

static void DrawWall(float x, float y, bool isVertical)
{
    Vector2 extents = (isVertical) ? (Vector2){0, WALL_SIZE / 2} : (Vector2){WALL_SIZE / 2, 0};
    AddLine(x - extents.x, y - extents.y, x + extents.x, y + extents.y, BLUE);
}

static void DrawDoor(float x, float y, bool isVertical)
{
    Vector2 extents = (isVertical) ? (Vector2){DOOR_THICKNESS / 2, DOOR_SIZE / 2} : (Vector2){DOOR_SIZE / 2, DOOR_THICKNESS / 2};
    AddLine(x - extents.x, y - extents.y, x + extents.x, y - extents.y, YELLOW);
    AddLine(x + extents.x, y - extents.y, x + extents.x, y + extents.y, YELLOW);
    AddLine(x + extents.x, y + extents.y, x - extents.x, y + extents.y, YELLOW);
    AddLine(x - extents.x, y + extents.y, x - extents.x, y - extents.y, YELLOW);

    if (isVertical)
    {
        AddLine(x, y - WALL_SIZE / 2, x, y - DOOR_SIZE / 2, BLUE);
        AddLine(x, y + DOOR_SIZE / 2, x, y + WALL_SIZE / 2, BLUE);
    }
    else
    {
        AddLine(x - WALL_SIZE / 2, y, x - DOOR_SIZE / 2, y, BLUE);
        AddLine(x + DOOR_SIZE / 2, y, x + WALL_SIZE / 2, y, BLUE);
    }
}

static void DrawRobot(float x, float y)
{
    // Upper body.
    AddLine(x, y, x - 16, y, LIME);
    AddLine(x - 16, y, x - 8, y - 16, LIME);
    AddLine(x - 8, y - 16, x + 8, y - 16, LIME);
    AddLine(x + 8, y - 16, x + 16, y, LIME);
    AddLine(x + 16, y, x, y, LIME);

    // Eyes.
    AddLine(x - 8, y - 6, x - 4, y - 6, RED);
    AddLine(x, y - 6, x + 4, y - 6, RED);

    // Lower body.
    AddLine(x - 12, y + 2, x + 12, y + 2, DARKGREEN);
    AddLine(x - 8, y + 4, x + 8, y + 4, DARKGREEN);
}

static void DrawPlayer(float x, float y)
{
    const Color bodyColour = SKYBLUE;
    const Color headColour = SKYBLUE;
    const Color legColour = SKYBLUE;

    // Body.
    AddLine(x, y, x, y - 12, bodyColour);

    // Arms.
    AddLine(x, y - 12, x - 8, y, bodyColour);
    AddLine(x, y - 12, x + 8, y, bodyColour);

    // Legs.
    AddLine(x, y, x - 8, y + 16, legColour);
    AddLine(x, y, x + 8, y + 16, legColour);

    // Head.
    AddLine(x, y - 12, x - 6, y - 16, headColour);
    AddLine(x, y - 12, x + 6, y - 16, headColour);
    AddLine(x - 6, y - 16, x, y - 20, headColour);
    AddLine(x + 6, y - 16, x, y - 20, headColour);
}

static void DrawShot(Vector2 position, Vector2 velocity)
{
    const Color shotColour = RED;

    const Vector2 shotLength = Vector2Scale(velocity, 3.0f);
    const Vector2 behind = Vector2Subtract(position, shotLength);

    AddLineV(behind, position, shotColour);
}

static void DrawWalls(RoomId roomId)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmWall | bmPosition | bmDrawable))
        {
            const Room* owner = Rooms.Get(id);
            if (owner->roomId == roomId)
            {
                Vector2 position = Positions.GetPosition(id);
                bool isVertical = Walls.IsVertical(id);
                DrawWall(position.x, position.y, isVertical);
            }
        }
    }
}

static void DrawDoors(RoomId roomId)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmDoor | bmPosition | bmDrawable))
        {
            const Room* owner = Rooms.Get(id);
            if (owner->roomId == roomId)
            {
                Vector2 position = Positions.GetPosition(id);
                bool isVertical = Doors.IsVertical(id);
                DrawDoor(position.x, position.y, isVertical);
            }
        }
    }
}

static void DrawRobots(RoomId roomId)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmRobot | bmPosition | bmDrawable))
        {
            const Room* owner = Rooms.Get(id);
            if (owner->roomId == roomId)
            {
                Vector2 position = Positions.GetPosition(id);
                DrawRobot(position.x, position.y);
            }
        }
    }
}

static void DrawPlayers(void)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmPlayer | bmPosition | bmDrawable))
        {
            Vector2 position = Positions.GetPosition(id);
            DrawPlayer(position.x, position.y);

            Vector2 aim = Aims.GetAim(id);
            if (aim.x != 0.0f || aim.y != 0.0f)
            {
                const Vector2 centre = Vector2Add(position, Vector2Scale(aim, 96.0f));
                DrawCircleLines(centre.x, centre.y, 8.0f, SKYBLUE);
            }
        }
    }
}

static void DrawShots(void)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmShot | bmPosition | bmDrawable | bmVelocity))
        {
            Vector2 position = Positions.GetPosition(id);
            Vector2 velocity = Velocities.GetVelocity(id);
            DrawShot(position, velocity);
        }
    }
}

static void DrawRoomById(RoomId roomId, Camera2D camera)
{
    BeginMode2D(camera);
    BeginDrawLines();
    DrawWalls(roomId);
    if (currentRoom == nextRoom)
    {
        DrawDoors(roomId);
    }
    DrawRobots(roomId);
    DrawPlayers();
    DrawShots();
    EndDrawLines();
    EndMode2D();
}

static void DrawRoom(void)
{
    const float roomX = ((float)GetScreenWidth() - 5.0f * WALL_SIZE) / 2.0f;
    const float roomY = 28.0f;
    Vector2 room = {roomX, roomY};

    if (currentRoom == nextRoom)
    {
        DrawRoomById(currentRoom, (Camera2D){.zoom = 1.0f, .offset = room});
    }
    else
    {
        // We're transitioning between two rooms, so draw both of them relative to how far we are into the transition.
        const GameTime transitionTime = 0.5;
        float alpha = (float)((GetTime() - exitTime) / transitionTime);
        if (alpha > 1.0f)
        {
            alpha = 1.0f;
        }

        Vector2 disp = Vector2Scale(secondCamera, alpha);
        DrawRoomById(currentRoom, (Camera2D){.zoom = 1.0f, .offset = Vector2Subtract(room, disp)});
        DrawRoomById(nextRoom, (Camera2D){.zoom = 1.0f, .offset = Vector2Add(room, Vector2Subtract(secondCamera, disp))});
    }
}

static void DrawHud(void)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmPlayer))
        {
            PlayerStatus* p = PlayerStatuses.Get(id);
            DrawText(TextFormat("Score: %d", p->score), 96, 4, 20, SKYBLUE);
            DrawText(TextFormat("Lives: %d", p->lives), GetScreenWidth() - 192, 4, 20, SKYBLUE);
        }
    }
}

static void HandleEvents(int first, int last)
{
    for (int i = first; i != last; i++)
    {
        const Event* e = Events.Get((EventId){.id = i});
        if (e->type == etDOOR)
        {
            const DoorEvent* de = &e->door;
            // If exiting, start a transition.
            if (de->type == deEXIT)
            {
                exitTime = GetTime();
                switch (de->entrance)
                {
                case fromLEFT:
                    secondCamera = (Vector2){WALL_SIZE * HWALLS, 0.0f};
                    break;
                case fromRIGHT:
                    secondCamera = (Vector2){-WALL_SIZE * HWALLS, 0.0f};
                    break;
                case fromTOP:
                    secondCamera = (Vector2){0.0f, WALL_SIZE * VWALLS};
                    break;
                case fromBOTTOM:
                    secondCamera = (Vector2){0.0f, -WALL_SIZE * VWALLS};
                    break;
                }
                nextRoom = de->entering;
                currentRoom = de->exiting;
            }
            else if (de->type == deENTER)
            {
                currentRoom = nextRoom;
            }
        }
    }
}

void ResetDrawingSystem(void)
{
    EventSystem.Register(HandleEvents);
    nextRoom = 0;
    currentRoom = 0;
    secondCamera = Vector2Zero();
}

void UpdateDrawingSystem(void)
{
    DrawRoom();
    DrawHud();
}
