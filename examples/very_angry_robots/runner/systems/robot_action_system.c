#include "robot_action_system.h"

#include "factory.h"
#include "geometry.h"
#include "queries.h"
#include "systems/event_system.h"
#include "tables/collidables.h"
#include "tables/events.h"
#include "tables/guests.h"
#include "tables/positions.h"
#include "tables/steps.h"
#include "tables/velocities.h"

#define QUANTUM 1024

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

typedef enum Syscalls
{
    SYSCALL_EXIT,                // The robot's program has finished.
    SYSCALL_GET_MY_POSITION,     // Gets the robot's position in the world.
    SYSCALL_GET_PLAYER_POSITION, // Gets the player's position in the world.
    SYSCALL_FIRE_AT,             // Fires a shot towards the given postion.
    SYSCALL_MOVE_TOWARDS,        // Instructs the robot to move towards the given position.
    SYSCALL_STOP,                // Instructs the robot to stop.
    SYSCALL_RAYCAST_TOWARDS      // Fires a ray towards the given position to detect obstacles.
} Syscalls;

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

static void HandleTrap(Guest* guest, const ArvissTrap* trap, EntityId id)
{
    // Check for a syscall.
    if (trap->mcause == trENVIRONMENT_CALL_FROM_M_MODE)
    {
        // The syscall number is in a7 (x17).
        const uint32_t syscall = ArvissReadXReg(&guest->cpu, 17);

        // Service the syscall.
        bool syscallHandled = true;
        switch (syscall)
        {
        case SYSCALL_EXIT:
            // The exit code is in a0 (x10).
            break;
        case SYSCALL_GET_MY_POSITION: {
            const Vector2 position = Positions.GetPosition(id);

            // The VM address of the structure to place the result is in a0 (x10).
            const uint32_t a0 = ArvissReadXReg(&guest->cpu, 10);
            BusCode mc = bcOK;
            guest->cpu.bus.Write32(guest->cpu.bus.token, a0, *(uint32_t*)&position.x, &mc);
            guest->cpu.bus.Write32(guest->cpu.bus.token, a0 + 4, *(uint32_t*)&position.y, &mc);
            break;
        }
        case SYSCALL_GET_PLAYER_POSITION: {
            const Vector2 position = Positions.GetPosition(playerId);

            // The VM address of the structure to place the result is in a0 (x10).
            const uint32_t a0 = ArvissReadXReg(&guest->cpu, 10);
            BusCode mc = bcOK;
            guest->cpu.bus.Write32(guest->cpu.bus.token, a0, *(uint32_t*)&position.x, &mc);
            guest->cpu.bus.Write32(guest->cpu.bus.token, a0 + 4, *(uint32_t*)&position.y, &mc);
            break;
        }
        case SYSCALL_FIRE_AT: {
            // The VM address of the structure containing the target is in a0.
            const uint32_t a0 = ArvissReadXReg(&guest->cpu, 10);
            BusCode mc = bcOK;
            uint32_t x = guest->cpu.bus.Read32(guest->cpu.bus.token, a0, &mc);
            uint32_t y = guest->cpu.bus.Read32(guest->cpu.bus.token, a0 + 4, &mc);
            RkVector v = {.x = *(float*)&x, .y = *(float*)&y};
            FireAt(id, &v);
            break;
        }
        case SYSCALL_MOVE_TOWARDS: {
            // The VM address of the structure containing the target is in a0.
            const uint32_t a0 = ArvissReadXReg(&guest->cpu, 10);
            BusCode mc = bcOK;
            uint32_t x = guest->cpu.bus.Read32(guest->cpu.bus.token, a0, &mc);
            uint32_t y = guest->cpu.bus.Read32(guest->cpu.bus.token, a0 + 4, &mc);
            RkVector v = {.x = *(float*)&x, .y = *(float*)&y};
            MoveTowards(id, &v);
            break;
        }
        case SYSCALL_STOP:
            Stop(id);
            break;
        case SYSCALL_RAYCAST_TOWARDS: {
            // The VM address of the structure containing the target is in a0.
            const uint32_t a0 = ArvissReadXReg(&guest->cpu, 10);
            BusCode mc = bcOK;
            uint32_t x = guest->cpu.bus.Read32(guest->cpu.bus.token, a0, &mc);
            uint32_t y = guest->cpu.bus.Read32(guest->cpu.bus.token, a0 + 4, &mc);
            RkVector v = {.x = *(float*)&x, .y = *(float*)&y};
            // The maximum distance is a float held in a1.
            const uint32_t distance = ArvissReadXReg(&guest->cpu, 11);
            const RkResult hit = RaycastTowards(id, &v, *(float*)&distance);
            ArvissWriteXReg(&guest->cpu, 10, hit == RK_HIT ? 1 : 0);
            break;
        }
        default:
            // Unknown syscall.
            TraceLog(LOG_WARNING, "Unknown syscall %04x", syscall);
            syscallHandled = false;
            break;
        }

        // If we handled the syscall then perform an MRET so that we can return from the trap.
        if (syscallHandled)
        {
            ArvissMret(&guest->cpu);
        }
    }
    else if (trap->mcause == trILLEGAL_INSTRUCTION)
    {
        TraceLog(LOG_WARNING, "Illegal instruction %8x", trap->mtval);
    }
    else
    {
        // Ideally I'd like this to be an error, but LOG_ERROR in raylib actually stops the program.
        TraceLog(LOG_WARNING, "Trap cause %d not recognised", trap->mcause);
    }
}

static void UpdateRobotGuest(EntityId id)
{
    Guest* guest = Guests.Get(id);
    ArvissResult result = ArvissRun(&guest->cpu, QUANTUM);
    if (ArvissResultIsTrap(result))
    {
        const ArvissTrap trap = ArvissResultAsTrap(result);
        HandleTrap(guest, &trap, id);
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
            if (Entities.Is(id, bmStepped))
            {
                const Step* s = Steps.Get(id);
                if (s->step != 0)
                {
                    continue;
                }
            }
            UpdateRobotGuest(id);
        }
    }
}
