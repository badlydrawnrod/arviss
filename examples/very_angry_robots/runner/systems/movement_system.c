#include "movement_system.h"

#include "entities.h"
#include "geometry.h"
#include "queries.h"
#include "systems/event_system.h"
#include "tables/collidables.h"
#include "tables/events.h"
#include "tables/owners.h"
#include "tables/positions.h"
#include "tables/steps.h"
#include "tables/velocities.h"

static bool isEnabled = true;
static EntityId playerId = {.id = -1};
static struct
{
    Vector2 v;
} desired[MAX_ENTITIES];

static void GetDesiredMovements(void)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        desired[i].v = Vector2Zero();
        EntityId id = {i};
        if (Entities.Is(id, bmPosition | bmVelocity))
        {
            if (Entities.Is(id, bmStepped))
            {
                const Step* s = Steps.Get(id);
                if (s->step != 0)
                {
                    continue;
                }
            }
            desired[i].v = Velocities.Get(id)->velocity;
        }
    }
}

static void CollidePlayer(void)
{
    if (!Entities.Is(playerId, bmCollidable))
    {
        return;
    }
    AABB playerAABB = Collidables.GetGeometry(playerId);
    playerAABB.centre = Positions.GetPosition(playerId);
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
            AABB otherAABB = Collidables.GetGeometry(id);
            otherAABB.centre = Positions.GetPosition(id);
            float t;
            if (CheckCollisionMovingAABBs(playerAABB, otherAABB, desired[playerId.id].v, desired[i].v, &t))
            {
                Events.Add(
                        &(Event){.type = etCOLLISION, .collision = (CollisionEvent){.t = t, .firstId = playerId, .secondId = id}});
            }
        }
    }
}

static void CollideRobot(EntityId robotId)
{
    AABB robotAABB = Collidables.GetGeometry(robotId);
    robotAABB.centre = Positions.GetPosition(robotId);
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
            AABB otherAABB = Collidables.GetGeometry(id);
            otherAABB.centre = Positions.GetPosition(id);
            float t;
            if (CheckCollisionMovingAABBs(robotAABB, otherAABB, desired[robotId.id].v, desired[i].v, &t))
            {
                Events.Add(
                        &(Event){.type = etCOLLISION, .collision = (CollisionEvent){.t = t, .firstId = robotId, .secondId = id}});
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
    AABB shotAABB = Collidables.GetGeometry(shotId);
    shotAABB.centre = Positions.GetPosition(shotId);
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
            AABB otherAABB = Collidables.GetGeometry(id);
            otherAABB.centre = Positions.GetPosition(id);
            float t;
            if (CheckCollisionMovingAABBs(shotAABB, otherAABB, desired[shotId.id].v, desired[i].v, &t))
            {
                desired[shotId.id].v = Vector2Scale(desired[shotId.id].v, t);
                Events.Add(&(Event){.type = etCOLLISION, .collision = (CollisionEvent){.t = t, .firstId = shotId, .secondId = id}});
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

static void CheckCollisions(void)
{
    CollidePlayer();
    CollideRobots();
    CollideShots();
}

static void ApplyDesiredMovements(void)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmPosition | bmVelocity))
        {
            Position* c = Positions.Get(id);
            Vector2 v = desired[i].v;
            c->position = Vector2Add(c->position, v);
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

void ResetMovementSystem(void)
{
    isEnabled = true;
    EventSystem.Register(HandleEvents);
}

void UpdateMovementSystem(void)
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

    GetDesiredMovements();
    CheckCollisions();
    ApplyDesiredMovements();
}
