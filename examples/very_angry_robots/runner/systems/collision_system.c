#include "collision_system.h"

#include "components/collidable_components.h"
#include "components/positions.h"
#include "components/velocities.h"
#include "entities.h"
#include "raylib.h"

static int playerId = -1;

static Rectangle geometries[] = {
        {.x = -8, .y = -8, .width = 16, .height = 16}, // ctROBOT
        {.x = -8, .y = -8, .width = 16, .height = 16}, // ctPLAYER
        {.x = -8, .y = -8, .width = 16, .height = 16}, // ctHWALL
        {.x = -8, .y = -8, .width = 16, .height = 16}, // ctVWALL
        {.x = -8, .y = -8, .width = 16, .height = 16}, // ctHDOOR
        {.x = -8, .y = -8, .width = 16, .height = 16}  // ctVDOOR
};

// TODO: obviously we'd want to cache some of these queries, otherwise we're going to spend our entire adult lives looping
//  through all of the entities. We'll also want to look at restricting things by position as there's no point in checking for
//  collisions with something that's on the other side of the world.

static void CollidePlayer(void)
{
    Vector2 playerPos = Positions.GetPosition(playerId);
    Rectangle playerRect = geometries[ctPLAYER];
    playerRect.x += playerPos.x;
    playerRect.y += playerPos.y;

    for (int id = 0, numEntities = Entities.Count(); id < numEntities; id++)
    {
        // TODO: how do we distinguish between player shots and robot shots?
        // TODO: do we have to separate static and dynamic components?

        // const bool shouldTest = Entities.Is(id, bmCollidable)
        //  && (Entities.Is(id, bmWall) || Entities.Is(id, bmDoor) || Entities.Is(id, bmRobot) || Entities.Is(id,
        //  bmShot));
        const bool shouldTest = Entities.Is(id, bmCollidable | bmVelocity) && (Entities.Is(id, bmWall) || Entities.Is(id, bmDoor));
        if (shouldTest)
        {
            CollidableComponent* c = CollidableComponents.Get(id);
            Vector2 otherPos = Velocities.GetVelocity(id);
            Rectangle otherRect = geometries[c->type];
            otherRect.x += otherPos.x;
            otherRect.y += otherPos.y;
            if (CheckCollisionRecs(playerRect, otherRect))
            {
                TraceLog(LOG_INFO, "Player hit something");
            }
        }
    }
}

static void CollideRobot(int robotId)
{
    for (int id = 0, numEntities = Entities.Count(); id < numEntities; id++)
    {
        if (robotId == id)
        {
            continue;
        }
        // TODO: how do we distinguish between player shots and robot shots?
        const bool shouldTest = Entities.Is(id, bmWall) || Entities.Is(id, bmDoor) || Entities.Is(id, bmRobot)
                || Entities.Is(id, bmShot) || Entities.Is(id, bmPlayer);
        if (shouldTest)
        {
            // TODO: Compare their collision geometries and emit an event if they're in collision.
        }
    }
}

static void CollideRobots(void)
{
    for (int id = 0, numEntities = Entities.Count(); id < numEntities; id++)
    {
        if (Entities.Is(id, bmRobot))
        {
            CollideRobot(id);
        }
    }
}

static void CollideShot(int shotId)
{
    for (int id = 0, numEntities = Entities.Count(); id < numEntities; id++)
    {
        if (shotId == id)
        {
            continue;
        }
        const bool shouldTest = Entities.Is(id, bmWall) || Entities.Is(id, bmDoor);
        if (shouldTest)
        {
            // TODO: Compare their collision geometries and emit an event if they're in collision.
        }
    }
}

static void CollideShots(void)
{
    for (int id = 0, numEntities = Entities.Count(); id < numEntities; id++)
    {
        if (Entities.Is(id, bmShot))
        {
            CollideShot(id);
        }
    }
}

void UpdateCollisionSystem(void)
{
    // Cache the player id.
    if (playerId == -1)
    {
        for (int id = 0; id < MAX_ENTITIES; id++)
        {
            if (Entities.Is(id, bmPlayer))
            {
                playerId = id;
                break;
            }
        }
    }

    CollidePlayer();
    CollideRobots();
    CollideShots();
}
