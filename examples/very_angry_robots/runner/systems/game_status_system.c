#include "game_status_system.h"

#include "entities.h"
#include "raylib.h"
#include "systems/event_system.h"
#include "tables/aims.h"
#include "tables/collidables.h"
#include "tables/doors.h"
#include "tables/events.h"
#include "tables/player_controls.h"
#include "tables/player_status.h"
#include "tables/positions.h"
#include "tables/rooms.h"
#include "tables/velocities.h"
#include "tables/walls.h"
#include "timed_triggers.h"

#define HWALLS 5
#define VWALLS 3
#define WALL_SIZE 224
#define ARENA_WIDTH (HWALLS * WALL_SIZE)
#define ARENA_HEIGHT (VWALLS * WALL_SIZE)

static void SpawnPlayer(Vector2 spawnPoint);

static bool gameOver = false;
static bool alreadyDied = false;
static TimedTrigger restartTime = {0.0};
static TimedTrigger amnestyTime = {0.0};
static GameTime transitionEndTime = 0.0;
static Entrance entrance;
static Vector2 playerSpawnPoint;
static EntityId playerId;
static RoomId currentRoomId;
static RoomId nextRoomId;

static EntityId MakeRobot(RoomId owner, float x, float y)
{
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmDrawable | bmRobot | bmCollidable | bmInRoom); // No velocity to start. It isn't movable.
    Positions.Set(id, &(Position){.position = {x, y}});
    Velocities.Set(id, &(Velocity){.velocity = {1.0f, 0.0f}});
    Collidables.Set(id, &(Collidable){.type = ctROBOT});
    Owners.Set(id, &(Room){.roomId = owner});
    return id;
}

static EntityId MakePlayer(float x, float y)
{
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmVelocity | bmDrawable | bmPlayer | bmCollidable);
    Positions.Set(id, &(Position){.position = {x, y}});
    Velocities.Set(id, &(Velocity){.velocity = {0.0f, 0.0f}});
    PlayerStatuses.Set(id, &(PlayerStatus){.lives = 3, .score = 0});
    Collidables.Set(id, &(Collidable){.type = ctPLAYER});
    // TODO: here we have things set without a corresponding bit in the entity. This should probably change.
    PlayerControls.Set(id, &(PlayerControl){.movement = {0.0f, 0.0f}, .aim = {0.0f, 0.0f}, .fire = false});
    Aims.Set(id, &(Aim){.aim = {0.0f, 0.0f}});
    return id;
}

static EntityId MakeWall(RoomId owner, float x, float y, bool isVertical)
{
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmDrawable | bmWall | bmCollidable | bmInRoom);
    Positions.Set(id, &(Position){.position = {x, y}});
    Walls.Set(id, &(Wall){.vertical = isVertical});
    Collidables.Set(id, &(Collidable){.type = isVertical ? ctVWALL : ctHWALL});
    Owners.Set(id, &(Room){.roomId = owner});
    return id;
}

static EntityId MakeWallFromGrid(RoomId owner, int gridX, int gridY, bool isVertical)
{
    const float x = (float)gridX * WALL_SIZE + ((isVertical) ? 0 : WALL_SIZE / 2);
    const float y = (float)gridY * WALL_SIZE + ((isVertical) ? WALL_SIZE / 2 : 0);
    return MakeWall(owner, x, y, isVertical);
}

static EntityId MakeExit(RoomId owner, float x, float y, bool isVertical, Entrance entrance)
{
    // An exit is an invisible door.
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmDoor | bmCollidable | bmInRoom);
    Positions.Set(id, &(Position){.position = {x, y}});
    Doors.Set(id, &(Door){.vertical = isVertical, .leadsTo = entrance});

    // It shares the same collision characteristics as a wall.
    Collidables.Set(id, &(Collidable){.type = isVertical ? ctVWALL : ctHWALL, .isTrigger = true});
    Owners.Set(id, &(Room){.roomId = owner});
    return id;
}

static EntityId MakeExitFromGrid(RoomId owner, int gridX, int gridY, bool isVertical, Entrance entrance)
{
    const float x = (float)gridX * WALL_SIZE + ((isVertical) ? 0 : WALL_SIZE / 2);
    const float y = (float)gridY * WALL_SIZE + ((isVertical) ? WALL_SIZE / 2 : 0);
    return MakeExit(owner, x, y, isVertical, entrance);
}

static EntityId MakeDoor(RoomId owner, float x, float y, bool isVertical, Entrance entrance)
{
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmDrawable | bmDoor | bmCollidable | bmInRoom);
    Positions.Set(id, &(Position){.position = {x, y}});
    Doors.Set(id, &(Door){.vertical = isVertical, .leadsTo = entrance});
    Collidables.Set(id, &(Collidable){.type = isVertical ? ctVDOOR : ctHDOOR});
    Owners.Set(id, &(Room){.roomId = owner});
    return id;
}

static EntityId MakeDoorFromGrid(RoomId owner, int gridX, int gridY, bool isVertical, Entrance entrance)
{
    const float x = (float)gridX * WALL_SIZE + ((isVertical) ? 0 : WALL_SIZE / 2);
    const float y = (float)gridY * WALL_SIZE + ((isVertical) ? WALL_SIZE / 2 : 0);
    return MakeDoor(owner, x, y, isVertical, entrance);
}

static void DestroyRoom(RoomId roomId)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {.id = i};
        if (Entities.Is(id, bmInRoom))
        {
            Room* owner = Owners.Get(id);
            if (owner->roomId == roomId)
            {
                Entities.Set(id, bmReap);
            }
        }
    }
}

static void CreateRoom(Entrance entrance, RoomId roomId)
{
    TraceLog(LOG_DEBUG, "Creating room");

    const bool horizontal = false;
    const bool vertical = true;

    // The player's spawn point is based on the door they entered through.
    const float xDisp = 32.0f;
    const float yDisp = 32.0f;
    switch (entrance)
    {
    case fromTOP:
        playerSpawnPoint = (Vector2){.x = ARENA_WIDTH / 2, .y = yDisp};
        MakeDoorFromGrid(roomId, 2, 0, horizontal, fromBOTTOM);
        MakeExitFromGrid(roomId, 2, 3, horizontal, fromTOP);
        MakeExitFromGrid(roomId, 0, 1, vertical, fromRIGHT);
        MakeExitFromGrid(roomId, 5, 1, vertical, fromLEFT);
        break;
    case fromBOTTOM:
        playerSpawnPoint = (Vector2){.x = ARENA_WIDTH / 2, .y = ARENA_HEIGHT - yDisp};
        MakeExitFromGrid(roomId, 2, 0, horizontal, fromBOTTOM);
        MakeDoorFromGrid(roomId, 2, 3, horizontal, fromTOP);
        MakeExitFromGrid(roomId, 0, 1, vertical, fromRIGHT);
        MakeExitFromGrid(roomId, 5, 1, vertical, fromLEFT);
        break;
    case fromLEFT:
        playerSpawnPoint = (Vector2){.x = xDisp, .y = ARENA_HEIGHT / 2};
        MakeExitFromGrid(roomId, 2, 0, horizontal, fromBOTTOM);
        MakeExitFromGrid(roomId, 2, 3, horizontal, fromTOP);
        MakeDoorFromGrid(roomId, 0, 1, vertical, fromRIGHT);
        MakeExitFromGrid(roomId, 5, 1, vertical, fromLEFT);
        break;
    case fromRIGHT:
        playerSpawnPoint = (Vector2){.x = ARENA_WIDTH - xDisp, .y = ARENA_HEIGHT / 2};
        MakeExitFromGrid(roomId, 2, 0, horizontal, fromBOTTOM);
        MakeExitFromGrid(roomId, 2, 3, horizontal, fromTOP);
        MakeExitFromGrid(roomId, 0, 1, vertical, fromRIGHT);
        MakeDoorFromGrid(roomId, 5, 1, vertical, fromLEFT);
        break;
    }

    MakeRobot(roomId, ARENA_WIDTH / 4, ARENA_HEIGHT / 2);
    MakeRobot(roomId, 3 * ARENA_WIDTH / 4, ARENA_HEIGHT / 2);
    MakeRobot(roomId, ARENA_WIDTH / 2, ARENA_HEIGHT / 4);
    MakeRobot(roomId, ARENA_WIDTH / 2, 3 * ARENA_HEIGHT / 4);

    // Top walls.
    MakeWallFromGrid(roomId, 0, 0, horizontal);
    MakeWallFromGrid(roomId, 1, 0, horizontal);
    MakeWallFromGrid(roomId, 3, 0, horizontal);
    MakeWallFromGrid(roomId, 4, 0, horizontal);

    // Bottom walls.
    MakeWallFromGrid(roomId, 0, 3, horizontal);
    MakeWallFromGrid(roomId, 1, 3, horizontal);
    MakeWallFromGrid(roomId, 3, 3, horizontal);
    MakeWallFromGrid(roomId, 4, 3, horizontal);

    // Left walls.
    MakeWallFromGrid(roomId, 0, 0, vertical);
    MakeWallFromGrid(roomId, 0, 2, vertical);

    // Right walls.
    MakeWallFromGrid(roomId, 5, 0, vertical);
    MakeWallFromGrid(roomId, 5, 2, vertical);
}

static void SpawnPlayer(Vector2 spawnPoint)
{
    TraceLog(LOG_DEBUG, "Spawning player at (%3.1f, %3.1f)", spawnPoint.x, spawnPoint.y);
    Position* p = Positions.Get(playerId);
    p->position = spawnPoint;

    Entities.Set(playerId, bmDrawable | bmCollidable | bmVelocity);

    Events.Add(&(Event){.type = etPLAYER, .player = (PlayerEvent){.type = peSPAWNED, .id = playerId}});

    SetTimedTrigger(&amnestyTime, GetTime() + 2.5);
    TraceLog(LOG_DEBUG, "Amnesty starting");
}

static void HandleEvents(int first, int last)
{
    for (int i = first; i != last; i++)
    {
        const Event* e = Events.Get((EventId){.id = i});

        switch (e->type)
        {
        case etPLAYER: {
            const PlayerEvent* pe = &e->player;
            if (!alreadyDied && pe->type == peDIED)
            {
                alreadyDied = true;
                PlayerStatus* p = PlayerStatuses.Get(playerId);
                --p->lives;
                TraceLog(LOG_INFO, "Player died. Lives reduced to %d", p->lives);
                SetTimedTrigger(&restartTime, GetTime() + 5);
                Entities.Clear(pe->id, bmDrawable | bmCollidable | bmVelocity);

                // The robots should stop moving because the player has died.
                for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
                {
                    EntityId id = {.id = i};
                    if (Entities.Is(id, bmRobot))
                    {
                        Entities.Clear(id, bmVelocity);
                    }
                }
            }
            else if (pe->type == peSPAWNED)
            {
                TraceLog(LOG_INFO, "Player spawned");
            }

            break;
        }
        case etDOOR: {
            const DoorEvent* de = &e->door;
            TraceLog(LOG_INFO, "Door event: %s", de->type == deENTER ? "enter" : "exit");
            if (de->type == deEXIT)
            {
                const GameTime transitionTime = 0.5;
                nextRoomId = currentRoomId + 1;
                CreateRoom(de->entrance, nextRoomId);
                transitionEndTime = GetTime() + transitionTime;
                entrance = de->entrance;
            }
            break;
        }
        }
    }
}

bool IsGameOverGameStatusSystem(void)
{
    return gameOver;
}

void ResetGameStatusSystem(void)
{
    gameOver = false;
    EventSystem.Register(HandleEvents);
    alreadyDied = false;
    ClearTimedTrigger(&restartTime);
    ClearTimedTrigger(&amnestyTime);
    playerId = MakePlayer(0.0f, 0.0f);
    currentRoomId = 0;
    CreateRoom(fromBOTTOM, currentRoomId);
    SpawnPlayer(playerSpawnPoint);
}

void UpdateGameStatusSystem(void)
{
    alreadyDied = false;

    const GameTime now = GetTime();

    if (PollTimedTrigger(&restartTime, now))
    {
        PlayerStatus* p = PlayerStatuses.Get(playerId);
        if (p->lives > 0)
        {
            SpawnPlayer(playerSpawnPoint);
        }
        else
        {
            TraceLog(LOG_INFO, "Game Over");
            gameOver = true;
        }
    }

    if (PollTimedTrigger(&amnestyTime, now))
    {
        TraceLog(LOG_DEBUG, "Amnesty over");

        // The robots are allowed to move once the amnesty is over.
        for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
        {
            EntityId id = {.id = i};
            if (Entities.Is(id, bmRobot))
            {
                Entities.Set(id, bmVelocity);
            }
        }
    }

    if (transitionEndTime > 0.0)
    {
        if (now >= transitionEndTime)
        {
            transitionEndTime = 0.0;
            DestroyRoom(currentRoomId);
            Events.Add(&(Event){
                    .type = etDOOR,
                    .door = (DoorEvent){.type = deENTER, .entrance = entrance, .entering = nextRoomId, .exiting = currentRoomId}});
            currentRoomId = nextRoomId;
            SpawnPlayer(playerSpawnPoint);
        }
    }
}
