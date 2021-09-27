#include "collision_response_system.h"

#include "entities.h"
#include "raylib.h"
#include "systems/event_system.h"
#include "tables/collidables.h"
#include "tables/doors.h"
#include "tables/events.h"
#include "tables/positions.h"
#include "tables/rooms.h"
#include "tables/velocities.h"
#include "types.h"

// TODO: this needs to be in _one_ place.
#define NUM_PHYSICS_STEPS 8
#define ALPHA (1.0f / NUM_PHYSICS_STEPS)

static const char* Identify(EntityId id)
{
    if (Entities.Is(id, bmPlayer))
    {
        return "player";
    }
    else if (Entities.Is(id, bmRobot))
    {
        return "robot";
    }
    else if (Entities.Is(id, bmShot))
    {
        return "shot";
    }
    else if (Entities.Is(id, bmWall))
    {
        return "wall";
    }
    else if (Entities.Is(id, bmDoor))
    {
        const Collidable* c = Collidables.Get(id);
        return c->isTrigger ? "exit" : "door";
    }
    return "*** unknown ***";
}

static void CancelDuplicateEvents(const CollisionEvent* src, int first, int last)
{
    // Scan ahead in the events to see if this collision event occurred again later in this time step, and if so, remove it.
    for (int i = first; i != last; i++)
    {
        Event* e = Events.Get((EventId){.id = i});
        if (e->type == etCOLLISION)
        {
            const CollisionEvent* c = &e->collision;
            if ((src->firstId.id == c->firstId.id && src->secondId.id == c->secondId.id)
                || (src->firstId.id == c->secondId.id && src->secondId.id == c->firstId.id))
            {
                TraceLog(LOG_DEBUG, "Cancelled event %d", i);
                e->type = etCANCELLED;
            }
        }
    }
}

static void CancelLaterEventsForReapedEntities(const CollisionEvent* src, int first, int last)
{
    // Nothing to do if neither entity is tagged for reaping.
    const bool firstReaped = Entities.Is(src->firstId, bmReap);
    const bool secondReaped = Entities.Is(src->secondId, bmReap);
    if (!firstReaped && !secondReaped)
    {
        return;
    }

    // Look for collisions from a later pass involving either of the entities involved in this collision. If either of the entities
    // is reaped then remove it. We do this to prevent situations where a later collision really mustn't take place, e.g., when a
    // shot has been reaped as a result of hitting a wall then anything that happened to it in a later pass should be cancelled.
    for (int i = first; i != last; i++)
    {
        Event* e = Events.Get((EventId){.id = i});
        if (e->type == etCOLLISION)
        {
            const CollisionEvent* c = &e->collision;
            if (c->pass > src->pass)
            {
                if ((firstReaped && (src->firstId.id == c->firstId.id || src->firstId.id == c->secondId.id))
                    || (secondReaped && (src->secondId.id == c->firstId.id || src->secondId.id == c->secondId.id)))
                {
                    TraceLog(LOG_DEBUG, "Cancelled later event %d", i);
                    e->type = etCANCELLED;
                }
            }
        }
    }
}

static void MoveToPositionAtTimeOfImpact(EntityId id, int pass)
{
    const Velocity* v = Velocities.Get(id);
    const float alpha = ALPHA * (float)(NUM_PHYSICS_STEPS - pass);
    Position* p = Positions.Get(id);
    p->position = Vector2Subtract(p->position, Vector2Scale(v->velocity, alpha));
}

static void HandleEvents(int first, int last)
{
    for (int i = first; i != last; i++)
    {
        const Event* e = Events.Get((EventId){.id = i});
        if (e->type != etCOLLISION)
        {
            continue;
        }

        const CollisionEvent* c = &e->collision;

        CancelDuplicateEvents(c, i + 1, last);

        TraceLog(LOG_INFO, "Collision between %s and %s", Identify(c->firstId), Identify(c->secondId));

        // Don't remove the player, walls or doors.
        if (Entities.AnyOf(c->firstId, bmRobot | bmShot))
        {
            Entities.Set(c->firstId, bmReap);
            MoveToPositionAtTimeOfImpact(c->firstId, c->pass);
        }

        // Only remove mobile entities. We don't want to remove walls and doors (that happened).
        if (Entities.AnyOf(c->secondId, bmRobot | bmPlayer | bmShot))
        {
            Entities.Set(c->secondId, bmReap);
            MoveToPositionAtTimeOfImpact(c->secondId, c->pass);
        }

        // Handle collisions with the player.
        if (Entities.Is(c->firstId, bmPlayer))
        {
            const Collidable* collidable = Collidables.Get(c->secondId);
            if (!collidable->isTrigger)
            {
                // The player has died because they hit something that wasn't a mere trigger.
                Events.Add(&(Event){.type = etPLAYER, .player = (PlayerEvent){.type = peDIED, .id = c->firstId}});
            }
            else if (Entities.Is(c->secondId, bmDoor))
            {
                const Room* owner = Owners.Get(c->secondId);
                const Door* door = Doors.Get(c->secondId);
                Events.Add(&(Event){.type = etDOOR,
                                    .door = (DoorEvent){.type = deEXIT,
                                                        .entrance = door->leadsTo,
                                                        .exiting = owner->roomId,
                                                        .entering = owner->roomId + 1}}); // TODO: better way of assigning ids.
            }
        }

        CancelLaterEventsForReapedEntities(c, i + 1, last);
    }
}

void ResetCollisionResponseSystem(void)
{
    EventSystem.Register(HandleEvents);
}

void UpdateCollisionResponseSystem(void)
{
}
