#include "collision_system.h"

#include "entities.h"
#include "queries.h"
#include "raylib.h"
#include "systems/event_system.h"
#include "tables/collidables.h"
#include "tables/events.h"
#include "tables/owners.h"
#include "tables/positions.h"

#define WALL_SIZE 224
#define WALL_THICKNESS 2

#define DOOR_SIZE (WALL_SIZE - 80)
#define DOOR_THICKNESS 2

#define ROBOT_WIDTH 32
#define ROBOT_HEIGHT 16

#define PLAYER_WIDTH 32
#define PLAYER_HEIGHT 32

#define SHOT_WIDTH 4
#define SHOT_HEIGHT 4

static Rectangle geometries[] = {
        {.x = -ROBOT_WIDTH / 2, .y = -ROBOT_HEIGHT / 2, .width = ROBOT_WIDTH, .height = ROBOT_HEIGHT},     // ctROBOT
        {.x = -PLAYER_WIDTH / 2, .y = -PLAYER_HEIGHT / 2, .width = PLAYER_WIDTH, .height = PLAYER_HEIGHT}, // ctPLAYER
        {.x = -SHOT_WIDTH / 2, .y = -SHOT_HEIGHT / 2, .width = SHOT_WIDTH, .height = SHOT_HEIGHT},         // ctSHOT
        {.x = -WALL_SIZE / 2, .y = -WALL_THICKNESS / 2, .width = WALL_SIZE, .height = WALL_THICKNESS},     // ctHWALL
        {.x = -WALL_THICKNESS / 2, .y = -WALL_SIZE / 2, .width = WALL_THICKNESS, .height = WALL_SIZE},     // ctVWALL
        {.x = -WALL_SIZE / 2, .y = -WALL_THICKNESS / 2, .width = WALL_SIZE, .height = WALL_THICKNESS},     // ctHDOOR
        {.x = -WALL_THICKNESS / 2, .y = -WALL_SIZE / 2, .width = WALL_THICKNESS, .height = WALL_SIZE},     // ctVDOOR
};

static bool isEnabled = true;
static EntityId playerId = {-1};
static int pass = 0;

// TODO: obviously we'd want to cache some of these queries, otherwise we're going to spend our entire adult lives looping
//  through all of the entities. We'll also want to look at restricting things by position as there's no point in checking for
//  collisions with something that's on the other side of the world.

static void CollidePlayer(void)
{
    if (!Entities.Is(playerId, bmCollidable))
    {
        return;
    }

    Vector2 playerPos = Positions.GetPosition(playerId);
    Rectangle playerRect = geometries[ctPLAYER];
    playerRect.x += playerPos.x;
    playerRect.y += playerPos.y;

    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        bool shouldTest = Entities.Is(id, bmCollidable | bmPosition) && (Entities.AnyOf(id, bmWall | bmDoor | bmRobot | bmShot));

        // The player shouldn't collide with their own shots.
        if (Entities.Is(id, bmOwned | bmShot))
        {
            Owner* owner = Owners.Get(id);
            if (owner->ownerId.id == playerId.id)
            {
                // The player shouldn't collide with their own shots.
                shouldTest = false;
            }
        }
        if (shouldTest)
        {
            Collidable* c = Collidables.Get(id);
            Vector2 otherPos = Positions.GetPosition(id);
            Rectangle otherRect = geometries[c->type];
            otherRect.x += otherPos.x;
            otherRect.y += otherPos.y;
            if (CheckCollisionRecs(playerRect, otherRect))
            {
                Events.Add(&(Event){.type = etCOLLISION,
                                    .collision = (CollisionEvent){.pass = pass, .firstId = playerId, .secondId = id}});
            }
        }
    }
}

static void CollideRobot(EntityId robotId)
{
    Vector2 robotPos = Positions.GetPosition(robotId);
    Rectangle robotRect = geometries[ctPLAYER];
    robotRect.x += robotPos.x;
    robotRect.y += robotPos.y;

    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (robotId.id == id.id)
        {
            continue;
        }
        bool shouldTest = Entities.Is(id, bmCollidable | bmPosition) && Entities.AnyOf(id, bmWall | bmDoor | bmRobot | bmShot);

        // The robot shouldn't collide with their own shots.
        if (Entities.Is(id, bmOwned | bmShot))
        {
            Owner* owner = Owners.Get(id);
            if (owner->ownerId.id == robotId.id)
            {
                // The robot shouldn't collide with their own shots.
                shouldTest = false;
            }
        }
        if (shouldTest)
        {
            Collidable* c = Collidables.Get(id);
            Vector2 otherPos = Positions.GetPosition(id);
            Rectangle otherRect = geometries[c->type];
            otherRect.x += otherPos.x;
            otherRect.y += otherPos.y;
            if (CheckCollisionRecs(robotRect, otherRect))
            {
                Events.Add(&(Event){.type = etCOLLISION,
                                    .collision = (CollisionEvent){.pass = pass, .firstId = robotId, .secondId = id}});
            }
        }
    }
}

static void CollideRobots(void)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmRobot | bmCollidable))
        {
            CollideRobot(id);
        }
    }
}

static void CollideShot(EntityId shotId)
{
    Vector2 shotPos = Positions.GetPosition(shotId);
    Rectangle shotRect = geometries[ctSHOT];
    shotRect.x += shotPos.x;
    shotRect.y += shotPos.y;

    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (shotId.id == id.id)
        {
            continue;
        }
        const bool shouldTest = Entities.AnyOf(id, bmWall | bmDoor);
        if (shouldTest)
        {
            Collidable* c = Collidables.Get(id);
            Vector2 otherPos = Positions.GetPosition(id);
            Rectangle otherRect = geometries[c->type];
            otherRect.x += otherPos.x;
            otherRect.y += otherPos.y;
            if (CheckCollisionRecs(shotRect, otherRect))
            {
                Events.Add(&(Event){.type = etCOLLISION,
                                    .collision = (CollisionEvent){.pass = pass, .firstId = shotId, .secondId = id}});
            }
        }
    }
}

static void CollideShots(void)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmShot | bmCollidable))
        {
            CollideShot(id);
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
            isEnabled = de->type == deENTER;
        }
    }
}

void ResetCollisionSystem(void)
{
    isEnabled = true;
    EventSystem.Register(HandleEvents);
}

void UpdateCollisionSystem(int currentPass)
{
    // Cache the player id.
    if (playerId.id == -1)
    {
        playerId = Queries.GetPlayerId();
    }

    if (!isEnabled)
    {
        return;
    }

    pass = currentPass;

    CollidePlayer();
    CollideRobots();
    CollideShots();
}
