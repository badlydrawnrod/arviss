#include "robot_action_system.h"

#include "bitcasts.h"
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

typedef enum Syscalls
{
    SYSCALL_EXIT,                // The robot's program has finished.
    SYSCALL_YIELD,               // Give way.
    SYSCALL_GET_MY_POSITION,     // Gets the robot's position in the world.
    SYSCALL_GET_PLAYER_POSITION, // Gets the player's position in the world.
    SYSCALL_FIRE_AT,             // Fires a shot towards the given postion.
    SYSCALL_MOVE_TOWARDS,        // Instructs the robot to move towards the given position.
    SYSCALL_STOP,                // Instructs the robot to stop.
    SYSCALL_RAYCAST_TOWARDS      // Fires a ray towards the given position to detect obstacles.
} Syscalls;

static void FireAt(EntityId id, const Vector2 target)
{
    const Position* p = Positions.Get(id);
    const Vector2 robotPos = p->position;
    const float angle = atan2f(target.y - robotPos.y, target.x - robotPos.x);
    const Vector2 aim = {cosf(angle), sinf(angle)};
    const Room* room = Rooms.Get(id);
    MakeRobotShot(room->roomId, p->position, aim, id);
}

static void MoveTowards(EntityId id, const Vector2 target)
{
    const Vector2 robotPos = Positions.GetPosition(id);
    Velocity* v = Velocities.Get(id);
    const Vector2 desiredPos = Vector2MoveTowards(robotPos, target, ROBOT_SPEED);
    v->velocity = Vector2Subtract(desiredPos, robotPos);
}

static bool RaycastTowards(EntityId id, const Vector2 position, float maxDistance)
{
    AABB myAABB = Collidables.GetGeometry(id);
    myAABB.centre = Positions.GetPosition(id);
    const Vector2 direction = Vector2Subtract(Vector2MoveTowards(myAABB.centre, position, maxDistance), myAABB.centre);

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
                return true;
            }
        }
    }
    return false;
}

static inline void SysExit(Guest* guest, EntityId id)
{
    // The exit code is in a0 (x10).
    const uint32_t exitCode = ArvissReadXReg(&guest->cpu, abiA0);

    // Remove the guest from the entity, and free it.
    Entities.Clear(id, bmGuest);
    Guests.Free(id);
}

static inline void SysYield(Guest* guest, EntityId id)
{
}

static inline void SysGetMyPosition(Guest* guest, EntityId id)
{
    const Vector2 position = Positions.GetPosition(id);

    // The VM address of the structure to place the result is in a0 (x10).
    const uint32_t a0 = ArvissReadXReg(&guest->cpu, abiA0);
    BusCode mc = bcOK;
    guest->cpu.bus.Write32(guest->cpu.bus.token, a0, FloatAsU32(position.x), &mc);
    guest->cpu.bus.Write32(guest->cpu.bus.token, a0 + 4, FloatAsU32(position.y), &mc);
}

static inline void SysGetPlayerPosition(Guest* guest, EntityId id)
{
    const Vector2 position = Positions.GetPosition(playerId);

    // The VM address of the structure to place the result is in a0 (x10).
    const uint32_t a0 = ArvissReadXReg(&guest->cpu, abiA0);
    BusCode mc = bcOK;
    guest->cpu.bus.Write32(guest->cpu.bus.token, a0, FloatAsU32(position.x), &mc);
    guest->cpu.bus.Write32(guest->cpu.bus.token, a0 + 4, FloatAsU32(position.y), &mc);
}

static inline void SysFireAt(Guest* guest, EntityId id)
{
    // The VM address of the structure containing the target is in a0.
    const uint32_t a0 = ArvissReadXReg(&guest->cpu, abiA0);
    BusCode mc = bcOK;
    uint32_t x = guest->cpu.bus.Read32(guest->cpu.bus.token, a0, &mc);
    uint32_t y = guest->cpu.bus.Read32(guest->cpu.bus.token, a0 + 4, &mc);
    const Vector2 v = {.x = U32AsFloat(x), .y = U32AsFloat(y)};
    FireAt(id, v);
}

static inline void SysMoveTowards(Guest* guest, EntityId id)
{
    // The VM address of the structure containing the target is in a0.
    const uint32_t a0 = ArvissReadXReg(&guest->cpu, abiA0);
    BusCode mc = bcOK;
    uint32_t x = guest->cpu.bus.Read32(guest->cpu.bus.token, a0, &mc);
    uint32_t y = guest->cpu.bus.Read32(guest->cpu.bus.token, a0 + 4, &mc);
    const Vector2 v = {.x = U32AsFloat(x), .y = U32AsFloat(y)};
    MoveTowards(id, v);
}

static inline void SysStop(Guest* guest, EntityId id)
{
    Velocity* v = Velocities.Get(id);
    v->velocity = Vector2Zero();
}

static inline void SysRaycastTowards(Guest* guest, EntityId id)
{
    // The VM address of the structure containing the target is in a0.
    const uint32_t a0 = ArvissReadXReg(&guest->cpu, abiA0);
    BusCode mc = bcOK;
    uint32_t x = guest->cpu.bus.Read32(guest->cpu.bus.token, a0, &mc);
    uint32_t y = guest->cpu.bus.Read32(guest->cpu.bus.token, a0 + 4, &mc);
    const Vector2 v = {.x = U32AsFloat(x), .y = U32AsFloat(y)};
    // The maximum distance is a float held in a1.
    const uint32_t distance = ArvissReadXReg(&guest->cpu, abiA1);
    const bool hit = RaycastTowards(id, v, U32AsFloat(distance));
    ArvissWriteXReg(&guest->cpu, 10, BoolAsU32(hit));
}

static void HandleTrap(Guest* guest, const ArvissTrap* trap, EntityId id)
{
    // Check for a syscall.
    if (trap->mcause == trENVIRONMENT_CALL_FROM_M_MODE)
    {
        // The syscall number is in a7 (x17).
        const uint32_t syscall = ArvissReadXReg(&guest->cpu, abiA7);

        // Service the syscall.
        bool syscallHandled = true;
        switch (syscall)
        {
        case SYSCALL_EXIT:
            SysExit(guest, id);
            break;
        case SYSCALL_YIELD:
            SysYield(guest, id);
            break;
        case SYSCALL_GET_MY_POSITION:
            SysGetMyPosition(guest, id);
            break;
        case SYSCALL_GET_PLAYER_POSITION:
            SysGetPlayerPosition(guest, id);
            break;
        case SYSCALL_FIRE_AT:
            SysFireAt(guest, id);
            break;
        case SYSCALL_MOVE_TOWARDS:
            SysMoveTowards(guest, id);
            break;
        case SYSCALL_STOP:
            SysStop(guest, id);
            break;
        case SYSCALL_RAYCAST_TOWARDS:
            SysRaycastTowards(guest, id);
            break;
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
    int remaining = QUANTUM;
    while (remaining > 0)
    {
        ArvissResult result = ArvissRun(&guest->cpu, remaining);
        if (ArvissResultIsTrap(result))
        {
            const ArvissTrap trap = ArvissResultAsTrap(result);
            HandleTrap(guest, &trap, id);
            if (trap.mcause == trENVIRONMENT_CALL_FROM_M_MODE && ArvissReadXReg(&guest->cpu, abiA7) == SYSCALL_YIELD)
            {
                // The VM has voluntarily given up control.
                break;
            }
        }
        remaining -= guest->cpu.retired;
    }
}

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
        if (Entities.Is(id, bmRobot | bmPosition | bmVelocity | bmGuest))
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
