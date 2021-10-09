#include "robot_action_system.h"

#include "factory.h"
#include "geometry.h"
#include "queries.h"
#include "systems/event_system.h"
#include "tables/collidables.h"
#include "tables/events.h"
#include "tables/positions.h"
#include "tables/velocities.h"

#define ROBOT_SPEED 4.0f

static bool isEnabled = true;
static EntityId playerId = {.id = -1};

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

void ResetRobotActions(void)
{
    isEnabled = true;
    EventSystem.Register(HandleEvents);
}

// --- <syscalls>
// Note that the syscalls have the entity id, because ultimately they'll be invoked from a trap. The trap handler will know which
// entity is being worked on.

// Anything Rk is "robot kernel", to make it obvious. Note that GetMyPosition(), etc, aren't "Rk" because they're internal to the
// kernel - the callers know nothing about EntityId.

typedef int32_t RkResult;

const RkResult RK_OK = 0;
const RkResult RK_HIT = 1;

typedef struct RkVector
{
    float x;
    float y;
} RkVector;

static RkResult GetMyPosition(EntityId id, RkVector* position)
{
    const Vector2 myPos = Positions.GetPosition(id);
    *position = (RkVector){.x = myPos.x, .y = myPos.y};
    return RK_OK;
}

static RkResult GetPlayerPosition(EntityId id, RkVector* position)
{
    const Vector2 playerPos = Positions.GetPosition(playerId);
    *position = (RkVector){.x = playerPos.x, .y = playerPos.y};
    return RK_OK;
}

static RkResult FireAt(EntityId id, const RkVector* target)
{
    const Position* p = Positions.Get(id);
    const Vector2 robotPos = p->position;
    const float angle = atan2f(target->y - robotPos.y, target->x - robotPos.x);
    const Vector2 aim = {cosf(angle), sinf(angle)};
    const Room* room = Rooms.Get(id);
    MakeRobotShot(room->roomId, p->position, aim, id);
    return RK_OK;
}

static RkResult MoveTowards(EntityId id, const RkVector* target)
{
    const Vector2 robotPos = Positions.GetPosition(id);
    Velocity* v = Velocities.Get(id);
    Vector2 desiredPos = Vector2MoveTowards(robotPos, (Vector2){.x = target->x, .y = target->y}, ROBOT_SPEED);
    v->velocity = Vector2Subtract(desiredPos, robotPos);
    return RK_OK;
}

static RkResult Stop(EntityId id)
{
    Velocity* v = Velocities.Get(id);
    v->velocity = Vector2Zero();
    return RK_OK;
}

static RkResult RaycastTowards(EntityId id, const RkVector* position, float maxDistance)
{
    AABB myAABB = Collidables.GetGeometry(id);
    myAABB.centre = Positions.GetPosition(id);
    const Vector2 direction = Vector2Subtract(
            Vector2MoveTowards(myAABB.centre, (Vector2){.x = position->x, .y = position->y}, maxDistance), myAABB.centre);

    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId otherId = {.id = i};
        if (otherId.id == id.id)
        {
            continue;
        }
        const bool shouldTest = Entities.Is(otherId, bmCollidable | bmPosition) && Entities.AnyOf(otherId, bmWall | bmDoor);
        if (shouldTest)
        {
            AABB otherAABB = Collidables.GetGeometry(otherId);
            otherAABB.centre = Positions.GetPosition(otherId);
            float t;
            if (CheckCollisionMovingAABBs(myAABB, otherAABB, direction, Vector2Zero(), &t))
            {
                return RK_HIT;
            }
        }
    }
    return RK_OK;
}

// --- </syscalls>

// Some shims to let us write "robot code" before we have the VM.

static EntityId currentEntity;

static inline RkResult RkGetMyPosition(RkVector* position)
{
    return GetMyPosition(currentEntity, position);
}

static inline RkResult RkGetPlayerPosition(RkVector* position)
{
    return GetPlayerPosition(currentEntity, position);
}

static inline RkResult RkFireAt(const RkVector* target)
{
    return FireAt(currentEntity, target);
}

static inline RkResult RkMoveTowards(const RkVector* target)
{
    return MoveTowards(currentEntity, target);
}

static inline RkResult RkStop(void)
{
    return Stop(currentEntity);
}

static inline RkResult RkRaycastTowards(const RkVector* position, float maxDistance)
{
    return RaycastTowards(currentEntity, position, maxDistance);
}

// --- <interact only through syscalls>

// This code is pretending to be a VM, so it can only interact with the world through syscalls.

static void UpdateRobot(void)
{
    // If this was running in the VM then it'd probably be in a while loop and it could have its own state.

    // Where am I?
    RkVector myPosition;
    RkGetMyPosition(&myPosition);

    // Where's the player?
    RkVector playerPosition;
    RkGetPlayerPosition(&playerPosition);

    const float probeDistance = 16.0f;

    // Is there anything that I might hit between me and the player?
    if (RkRaycastTowards(&playerPosition, probeDistance) != RK_HIT)
    {
        // Move in the direction of the player.
        RkMoveTowards(&playerPosition);
    }
    else if (RkRaycastTowards(&(RkVector){.x = playerPosition.x, .y = 0.0f}, probeDistance) != RK_HIT)
    {
        // Move horizontally towards the player.
        RkMoveTowards(&(RkVector){.x = playerPosition.x, .y = 0.0f});
    }
    else if (RkRaycastTowards(&(RkVector){.x = 0.0f, .y = playerPosition.y}, probeDistance) != RK_HIT)
    {
        // Move vertically towards the player.
        RkMoveTowards(&(RkVector){.x = 0.0f, .y = playerPosition.y});
    }
    else
    {
        // We can't move without hitting anything, so stop.
        RkStop();
    }

    // Fire at the player.
    if (((int)myPosition.x % 47) == 0)
    {
        RkFireAt(&playerPosition);
    }
}

// --- </interact only through syscalls>

void UpdateRobotActions(void)
{
    if (!isEnabled)
    {
        return;
    }

    // Cache the player id.
    if (playerId.id == -1)
    {
        playerId = Queries.GetPlayerId();
    }

    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmRobot | bmPosition | bmVelocity))
        {
            currentEntity = id;
            UpdateRobot();
        }
    }
}
