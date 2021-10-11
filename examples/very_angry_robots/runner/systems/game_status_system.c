#include "game_status_system.h"

#include "entities.h"
#include "factory.h"
#include "raylib.h"
#include "systems/event_system.h"
#include "tables/doors.h"
#include "tables/events.h"
#include "tables/player_status.h"
#include "tables/positions.h"
#include "timed_triggers.h"

// TODO: find a common place for these (if sensible).
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

static void DestroyRoom(RoomId roomId)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {.id = i};
        if (Entities.Is(id, bmInRoom))
        {
            Room* owner = Rooms.Get(id);
            if (owner->roomId == roomId)
            {
                Entities.Set(id, bmReap);
            }
        }
    }
}

static void CreateRoom(Entrance entrance, RoomId roomId)
{
    const bool horizontal = false;
    const bool vertical = true;

    // The player's spawn point is based on the door they entered through.
    const float xDisp = 32.0f;
    const float yDisp = 32.0f;
    switch (entrance)
    {
    case fromTOP:
        playerSpawnPoint = (Vector2){.x = ARENA_WIDTH * 0.5f, .y = yDisp};
        MakeDoorFromGrid(roomId, 2, 0, horizontal, fromBOTTOM);
        MakeExitFromGrid(roomId, 2, 3, horizontal, fromTOP);
        MakeExitFromGrid(roomId, 0, 1, vertical, fromRIGHT);
        MakeExitFromGrid(roomId, 5, 1, vertical, fromLEFT);
        break;
    case fromBOTTOM:
        playerSpawnPoint = (Vector2){.x = ARENA_WIDTH * 0.5f, .y = ARENA_HEIGHT - yDisp};
        MakeExitFromGrid(roomId, 2, 0, horizontal, fromBOTTOM);
        MakeDoorFromGrid(roomId, 2, 3, horizontal, fromTOP);
        MakeExitFromGrid(roomId, 0, 1, vertical, fromRIGHT);
        MakeExitFromGrid(roomId, 5, 1, vertical, fromLEFT);
        break;
    case fromLEFT:
        playerSpawnPoint = (Vector2){.x = xDisp, .y = ARENA_HEIGHT * 0.5f};
        MakeExitFromGrid(roomId, 2, 0, horizontal, fromBOTTOM);
        MakeExitFromGrid(roomId, 2, 3, horizontal, fromTOP);
        MakeDoorFromGrid(roomId, 0, 1, vertical, fromRIGHT);
        MakeExitFromGrid(roomId, 5, 1, vertical, fromLEFT);
        break;
    case fromRIGHT:
        playerSpawnPoint = (Vector2){.x = ARENA_WIDTH - xDisp, .y = ARENA_HEIGHT * 0.5f};
        MakeExitFromGrid(roomId, 2, 0, horizontal, fromBOTTOM);
        MakeExitFromGrid(roomId, 2, 3, horizontal, fromTOP);
        MakeExitFromGrid(roomId, 0, 1, vertical, fromRIGHT);
        MakeDoorFromGrid(roomId, 5, 1, vertical, fromLEFT);
        break;
    }

    MakeRobot(roomId, ARENA_WIDTH * 0.25f, ARENA_HEIGHT * 0.5f);
    MakeRobot(roomId, 3 * ARENA_WIDTH * 0.25f, ARENA_HEIGHT * 0.5f);
    MakeRobot(roomId, ARENA_WIDTH * 0.5f, ARENA_HEIGHT * 0.25f);
    MakeRobot(roomId, ARENA_WIDTH * 0.5f, 3 * ARENA_HEIGHT * 0.25f);

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

    // Another wall to separate the top 3 robots from the player.
    MakeWallFromGrid(roomId, 1, 2, horizontal);
    MakeWallFromGrid(roomId, 2, 2, horizontal);
    MakeWallFromGrid(roomId, 3, 2, horizontal);
}

static void SpawnPlayer(Vector2 spawnPoint)
{
    Position* p = Positions.Get(playerId);
    p->position = spawnPoint;

    Entities.Set(playerId, bmDrawable | bmCollidable | bmVelocity);

    Events.Add(&(Event){.type = etPLAYER, .player = (PlayerEvent){.type = peSPAWNED, .id = playerId}});

    SetTimedTrigger(&amnestyTime, GetTime() + 2.5);
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
                SetTimedTrigger(&restartTime, GetTime() + 5);
                Entities.Clear(pe->id, bmDrawable | bmCollidable | bmVelocity);

                // The robots should stop moving because the player has died.
                for (int j = 0, numEntities = Entities.MaxCount(); j < numEntities; j++)
                {
                    EntityId id = {.id = j};
                    if (Entities.Is(id, bmRobot))
                    {
                        Entities.Clear(id, bmVelocity);
                    }
                }
            }

            break;
        }
        case etDOOR: {
            const DoorEvent* de = &e->door;
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
            gameOver = true;
        }
    }

    if (PollTimedTrigger(&amnestyTime, now))
    {
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
