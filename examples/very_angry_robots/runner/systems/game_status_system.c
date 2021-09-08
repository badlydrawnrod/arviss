#include "game_status_system.h"

#include "components/collidables.h"
#include "components/doors.h"
#include "components/events.h"
#include "components/player_status.h"
#include "components/positions.h"
#include "components/velocities.h"
#include "components/walls.h"
#include "entities.h"
#include "raylib.h"
#include "systems/event_system.h"
#include "timed_triggers.h"

#define HWALLS 5
#define VWALLS 3
#define WALL_SIZE 224
#define ARENA_WIDTH (HWALLS * WALL_SIZE)
#define ARENA_HEIGHT (VWALLS * WALL_SIZE)

static void SpawnPlayer(void);
static void CreateRoom(void);

static bool gameOver = false;
static bool alreadyDied = false;
static TimedTrigger restartTime = {0.0};
static TimedTrigger amnestyTime = {0.0};
static Vector2 playerSpawnPoint;
static EntityId playerId;

static EntityId MakeRobot(float x, float y)
{
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmDrawable | bmRobot | bmCollidable); // No velocity to start. It isn't movable.
    Positions.Set(id, &(Position){.position = {x, y}});
    Velocities.Set(id, &(Velocity){.velocity = {1.0f, 0.0f}});
    Collidables.Set(id, &(Collidable){.type = ctROBOT});
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
    return id;
}

static EntityId MakeWall(float x, float y, bool isVertical)
{
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmDrawable | bmWall | bmCollidable);
    Positions.Set(id, &(Position){.position = {x, y}});
    Walls.Set(id, &(Wall){.vertical = isVertical});
    Collidables.Set(id, &(Collidable){.type = isVertical ? ctVWALL : ctHWALL});
    return id;
}

static EntityId MakeWallFromGrid(int gridX, int gridY, bool isVertical)
{
    const float x = (float)gridX * WALL_SIZE + ((isVertical) ? 0 : WALL_SIZE / 2);
    const float y = (float)gridY * WALL_SIZE + ((isVertical) ? WALL_SIZE / 2 : 0);
    return MakeWall(x, y, isVertical);
}

static EntityId MakeDoor(float x, float y, bool isVertical)
{
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmDrawable | bmDoor | bmCollidable);
    Positions.Set(id, &(Position){.position = {x, y}});
    Doors.Set(id, &(Door){.vertical = isVertical});
    Collidables.Set(id, &(Collidable){.type = isVertical ? ctVDOOR : ctHDOOR});
    return id;
}

static EntityId MakeDoorFromGrid(int gridX, int gridY, bool isVertical)
{
    const float x = (float)gridX * WALL_SIZE + ((isVertical) ? 0 : WALL_SIZE / 2);
    const float y = (float)gridY * WALL_SIZE + ((isVertical) ? WALL_SIZE / 2 : 0);
    return MakeDoor(x, y, isVertical);
}

static void CreateRoom(void)
{
    TraceLog(LOG_DEBUG, "Creating room");

    playerSpawnPoint = (Vector2){.x = ARENA_WIDTH / 2, .y = ARENA_HEIGHT / 2};

    playerId = MakePlayer(playerSpawnPoint.x, playerSpawnPoint.y);
    SpawnPlayer();

    MakeRobot(ARENA_WIDTH / 4, ARENA_HEIGHT / 2);
    MakeRobot(3 * ARENA_WIDTH / 4, ARENA_HEIGHT / 2);
    MakeRobot(ARENA_WIDTH / 2, ARENA_HEIGHT / 4);
    MakeRobot(ARENA_WIDTH / 2, 3 * ARENA_HEIGHT / 4);

    const bool horizontal = false;
    const bool vertical = true;

    // Top walls.
    MakeWallFromGrid(0, 0, horizontal);
    MakeWallFromGrid(1, 0, horizontal);
    MakeWallFromGrid(3, 0, horizontal);
    MakeWallFromGrid(4, 0, horizontal);

    // Bottom walls.
    MakeWallFromGrid(0, 3, horizontal);
    MakeWallFromGrid(1, 3, horizontal);
    MakeWallFromGrid(3, 3, horizontal);
    MakeWallFromGrid(4, 3, horizontal);

    // Left walls.
    MakeWallFromGrid(0, 0, vertical);
    MakeWallFromGrid(0, 2, vertical);

    // Right walls.
    MakeWallFromGrid(5, 0, vertical);
    MakeWallFromGrid(5, 2, vertical);

    // Doors.
    MakeDoorFromGrid(2, 0, horizontal);
    MakeDoorFromGrid(2, 3, horizontal);
    MakeDoorFromGrid(0, 1, vertical);
    MakeDoorFromGrid(5, 1, vertical);
}

static void SpawnPlayer(void)
{
    TraceLog(LOG_DEBUG, "Spawning player at (%3.1f, %3.1f)", playerSpawnPoint.x, playerSpawnPoint.y);
    Position* p = Positions.Get(playerId);
    p->position = playerSpawnPoint;

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
        if (e->type != etPLAYER)
        {
            continue;
        }

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
    CreateRoom();
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
            SpawnPlayer();
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
}
