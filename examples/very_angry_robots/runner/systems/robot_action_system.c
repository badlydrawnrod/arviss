#include "robot_action_system.h"

#include "factory.h"
#include "queries.h"
#include "systems/event_system.h"
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

static bool GetMyPosition(EntityId id, float* x, float* y)
{
    const Vector2 myPos = Positions.GetPosition(id);
    *x = myPos.x;
    *y = myPos.y;

    return true;
}

static bool GetPlayerPosition(EntityId id, float* x, float* y)
{
    const Vector2 playerPos = Positions.GetPosition(playerId);
    *x = playerPos.x;
    *y = playerPos.y;

    return true;
}

static bool FireAt(EntityId id, float x, float y)
{
    const Position* p = Positions.Get(id);
    const Vector2 robotPos = p->position;
    const float angle = atan2f(y - robotPos.y, x - robotPos.x);
    const Vector2 aim = {cosf(angle), sinf(angle)};
    const Room* room = Rooms.Get(id);
    MakeRobotShot(room->roomId, p->position, aim, id);

    return true;
}

static bool MoveTowards(EntityId id, float x, float y)
{
    const Position* p = Positions.Get(id);
    const Vector2 robotPos = p->position;
    const float angle = atan2f(y - robotPos.y, x - robotPos.x);
    const Vector2 movement = {cosf(angle), sinf(angle)};

    // Given that the robot's movement is stepped, it would be more accurate to call this "velocity when moved" or something
    // similar.
    Velocity* v = Velocities.Get(id);
    v->velocity = Vector2Scale(movement, ROBOT_SPEED);

    return true;
}

// --- </syscalls>

// --- <interact only through syscalls>
// This code is pretending to be a VM, so it can only interact with the world through syscalls.

static void UpdateRobot(EntityId id)
{
    // If this was running in the VM then it'd probably be in a while loop and it could have its own state.

    // Where am I?
    float myX;
    float myY;
    GetMyPosition(id, &myX, &myY);

    // Where's the player?
    float playerX;
    float playerY;
    GetPlayerPosition(id, &playerX, &playerY);

    // TODO: check to see if there's a wall between me and the player.

    // Move in the direction of the player.
    MoveTowards(id, playerX, playerY);

    // Fire at the player.
    if (((int)myX % 47) == 0)
    {
        FireAt(id, playerX, playerY);
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
            UpdateRobot(id);
        }
    }
}
