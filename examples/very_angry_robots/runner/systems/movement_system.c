#include "movement_system.h"

#include "entities.h"
#include "queries.h"
#include "systems/event_system.h"
#include "tables/collidables.h"
#include "tables/events.h"
#include "tables/owners.h"
#include "tables/positions.h"
#include "tables/steps.h"
#include "tables/velocities.h"

#define WALL_SIZE 224
#define WALL_THICKNESS 2

#define DOOR_SIZE (WALL_SIZE - 80)
#define DOOR_THICKNESS 2

#define ROBOT_WIDTH 32
#define ROBOT_HEIGHT 16

#define PLAYER_WIDTH 32
#define PLAYER_HEIGHT 32

#define SHOT_WIDTH 2
#define SHOT_HEIGHT 2

typedef struct
{
    Vector2 centre;
    Vector2 extents;
} AABB;

typedef struct
{
    Vector2 origin;
    Vector2 direction; // The direction is not necessarily normalized.
} Ray2D;

static AABB geometries[] = {
        {.centre = {.x = 0.0f, .y = 0.0f}, .extents = {.x = ROBOT_WIDTH * 0.5f, .y = ROBOT_HEIGHT * 0.5f}},   // ctROBOT
        {.centre = {.x = 0.0f, .y = 0.0f}, .extents = {.x = PLAYER_WIDTH * 0.5f, .y = PLAYER_HEIGHT * 0.5f}}, // ctPLAYER
        {.centre = {.x = 0.0f, .y = 0.0f}, .extents = {.x = SHOT_WIDTH * 0.5f, .y = SHOT_HEIGHT * 0.5f}},     // ctSHOT
        {.centre = {.x = 0.0f, .y = 0.0f}, .extents = {.x = WALL_SIZE * 0.5f, .y = WALL_THICKNESS * 0.5f}},   // ctHWALL
        {.centre = {.x = 0.0f, .y = 0.0f}, .extents = {.x = WALL_THICKNESS * 0.5f, .y = WALL_SIZE * 0.5f}},   // ctVWALL
        {.centre = {.x = 0.0f, .y = 0.0f}, .extents = {.x = WALL_SIZE * 0.5f, .y = WALL_THICKNESS * 0.5f}},   // ctHDOOR
        {.centre = {.x = 0.0f, .y = 0.0f}, .extents = {.x = WALL_THICKNESS * 0.5f, .y = WALL_SIZE * 0.5f}},   // ctVDOOR
};

static bool isEnabled = true;
static EntityId playerId = {.id = -1};
static Vector2 desired[MAX_ENTITIES];

static inline float Max(float a, float b)
{
    return a > b ? a : b;
}

static inline float Min(float a, float b)
{
    return a < b ? a : b;
}

inline Vector2 Vector2Rcp(Vector2 a)
{
    return (Vector2){.x = 1.0f / a.x, .y = 1.0f / a.y};
}

inline Vector2 Vector2Min(Vector2 a, Vector2 b)
{
    return (Vector2){Min(a.x, b.x), Min(a.y, b.y)};
}

inline Vector2 Vector2Max(Vector2 a, Vector2 b)
{
    return (Vector2){Max(a.x, b.x), Max(a.y, b.y)};
}

inline float Vector2MinComponent(Vector2 a)
{
    return Min(a.x, a.y);
}

inline float Vector2MaxComponent(Vector2 a)
{
    return Max(a.x, a.y);
}

// See: https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525
// https://gist.githubusercontent.com/bromanz/ed0de6725f5e40a0afd8f50985c2f7ad/raw/be5e79e16181e4617d1a0e6e540dd25c259c76a4/efficient-slab-test-majercik-et-al
inline bool Slabs(Vector2 p0, Vector2 p1, Vector2 rayOrigin, Vector2 invRayDir)
{
    const Vector2 t0 = Vector2Multiply(Vector2Subtract(p0, rayOrigin), invRayDir);
    const Vector2 t1 = Vector2Multiply(Vector2Subtract(p1, rayOrigin), invRayDir);
    const Vector2 tmin = Vector2Min(t0, t1);
    const Vector2 tmax = Vector2Max(t0, t1);
    return Max(0.0f, Vector2MaxComponent(tmin)) <= Min(1.0f, Vector2MinComponent(tmax));
}

bool CheckCollisionRay2dAABBs(Ray2D r, AABB aabb)
{
    const Vector2 invD = Vector2Rcp(r.direction);
    const Vector2 aabbMin = Vector2Subtract(aabb.centre, aabb.extents);
    const Vector2 aabbMax = Vector2Add(aabb.centre, aabb.extents);
    return Slabs(aabbMin, aabbMax, r.origin, invD);
}

bool CheckCollisionMovingAABBs(AABB a, AABB b, Vector2 va, Vector2 vb)
{
    // An AABB at B's position with the combined size of A and B.
    const AABB aabb = {.centre = b.centre, .extents = Vector2Add(a.extents, b.extents)};

    // A ray at A's position with its direction set to B's velocity relative to A. It's a parametric representation of a
    // line representing A's position at time t, where 0 <= t <= 1.
    const Ray2D r = {.origin = a.centre, .direction = Vector2Subtract(vb, va)};

    // Does the ray hit the AABB
    return CheckCollisionRay2dAABBs(r, aabb);
}

static void GetDesiredMovements(void)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmPosition | bmVelocity))
        {
            if (Entities.Is(id, bmStepped))
            {
                const Step* s = Steps.Get(id);
                if (s->step != 0)
                {
                    desired[i] = Vector2Zero();
                    continue;
                }
            }
            desired[i] = Velocities.Get(id)->velocity;
        }
    }
}

static void CollidePlayer(void)
{
    if (!Entities.Is(playerId, bmCollidable))
    {
        return;
    }
    AABB playerAABB = geometries[ctPLAYER];
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
            Collidable* c = Collidables.Get(id);
            AABB otherAABB = geometries[c->type];
            otherAABB.centre = Positions.GetPosition(id);
            if (CheckCollisionMovingAABBs(playerAABB, otherAABB, desired[playerId.id], desired[i]))
            {
                Events.Add(&(Event){.type = etCOLLISION, .collision = (CollisionEvent){.firstId = playerId, .secondId = id}});
            }
        }
    }
}

static void CollideRobot(EntityId robotId)
{
    AABB robotAABB = geometries[ctROBOT];
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
            Collidable* c = Collidables.Get(id);
            AABB otherAABB = geometries[c->type];
            otherAABB.centre = Positions.GetPosition(id);
            if (CheckCollisionMovingAABBs(robotAABB, otherAABB, desired[robotId.id], desired[i]))
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
        if (Entities.Is(id, bmRobot | bmCollidable))
        {
            CollideRobot(id);
        }
    }
}

static void CollideShot(EntityId shotId)
{
    AABB shotAABB = geometries[ctSHOT];
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
            Collidable* c = Collidables.Get(id);
            AABB otherAABB = geometries[c->type];
            otherAABB.centre = Positions.GetPosition(id);
            if (CheckCollisionMovingAABBs(shotAABB, otherAABB, desired[shotId.id], desired[i]))
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
            Vector2 v = desired[i];
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
