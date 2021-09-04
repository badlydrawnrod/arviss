#include "collision_system.h"

#include "components/collidable_components.h"
#include "components/events.h"
#include "components/positions.h"
#include "entities.h"
#include "raylib.h"

#define WALL_SIZE 224
#define WALL_THICKNESS 2

#define DOOR_SIZE (WALL_SIZE - 80)
#define DOOR_THICKNESS 2

#define ROBOT_WIDTH 32
#define ROBOT_HEIGHT 16

#define PLAYER_WIDTH 32
#define PLAYER_HEIGHT 32

static Rectangle geometries[] = {
        {.x = -ROBOT_WIDTH / 2, .y = -ROBOT_HEIGHT / 2, .width = ROBOT_WIDTH, .height = ROBOT_HEIGHT},     // ctROBOT
        {.x = -PLAYER_WIDTH / 2, .y = -PLAYER_HEIGHT / 2, .width = PLAYER_WIDTH, .height = PLAYER_HEIGHT}, // ctPLAYER
        {.x = -WALL_SIZE / 2, .y = -WALL_THICKNESS / 2, .width = WALL_SIZE, .height = WALL_THICKNESS},     // ctHWALL
        {.x = -WALL_THICKNESS / 2, .y = -WALL_SIZE / 2, .width = WALL_THICKNESS, .height = WALL_SIZE},     // ctVWALL
        {.x = -WALL_SIZE / 2, .y = -WALL_THICKNESS / 2, .width = WALL_SIZE, .height = WALL_THICKNESS},     // ctHDOOR
        {.x = -WALL_THICKNESS / 2, .y = -WALL_SIZE / 2, .width = WALL_THICKNESS, .height = WALL_SIZE},     // ctVDOOR
};

static EntityId playerId = {-1};

// TODO: obviously we'd want to cache some of these queries, otherwise we're going to spend our entire adult lives looping
//  through all of the entities. We'll also want to look at restricting things by position as there's no point in checking for
//  collisions with something that's on the other side of the world.

static void CollidePlayer(void)
{
    Vector2 playerPos = Positions.GetPosition(playerId);
    Rectangle playerRect = geometries[ctPLAYER];
    playerRect.x += playerPos.x;
    playerRect.y += playerPos.y;

    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        // TODO: how do we distinguish between player shots and robot shots?
        const bool shouldTest =
                Entities.Is(id, bmCollidable | bmPosition) && (Entities.AnyOf(id, bmWall | bmDoor | bmRobot | bmShot));
        if (shouldTest)
        {
            CollidableComponent* c = CollidableComponents.Get(id);
            Vector2 otherPos = Positions.GetPosition(id);
            Rectangle otherRect = geometries[c->type];
            otherRect.x += otherPos.x;
            otherRect.y += otherPos.y;
            if (CheckCollisionRecs(playerRect, otherRect))
            {
                Events.Add(&(Event){.type = etCOLLISION, .collision = (CollisionEvent){.firstId = playerId, .secondId = id}});
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
        const bool shouldTest =
                Entities.Is(id, bmCollidable | bmPosition) && Entities.AnyOf(id, bmWall | bmDoor | bmRobot | bmShot);
        if (shouldTest)
        {
            CollidableComponent* c = CollidableComponents.Get(id);
            Vector2 otherPos = Positions.GetPosition(id);
            Rectangle otherRect = geometries[c->type];
            otherRect.x += otherPos.x;
            otherRect.y += otherPos.y;
            if (CheckCollisionRecs(robotRect, otherRect))
            {
                Events.Add(&(Event){.type = etCOLLISION, .collision = (CollisionEvent){.firstId = robotId, .secondId = id}});
            }
        }
    }
}

static void CollideRobots(void)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmRobot))
        {
            CollideRobot(id);
        }
    }
}

static void CollideShot(EntityId shotId)
{
    Vector2 shotPos = Positions.GetPosition(shotId);
    Rectangle shotRect = geometries[ctPLAYER];
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
            CollidableComponent* c = CollidableComponents.Get(id);
            Vector2 otherPos = Positions.GetPosition(id);
            Rectangle otherRect = geometries[c->type];
            otherRect.x += otherPos.x;
            otherRect.y += otherPos.y;
            if (CheckCollisionRecs(shotRect, otherRect))
            {
                Events.Add(&(Event){.type = etCOLLISION, .collision = (CollisionEvent){.firstId = shotId, .secondId = id}});
            }
        }
    }
}

static void CollideShots(void)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmShot))
        {
            CollideShot(id);
        }
    }
}

void UpdateCollisionSystem(void)
{
    // Cache the player id.
    if (playerId.id == -1)
    {
        for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
        {
            if (Entities.Is((EntityId){i}, bmPlayer))
            {
                playerId.id = i;
                break;
            }
        }
    }

    CollidePlayer();
    CollideRobots();
    CollideShots();
}