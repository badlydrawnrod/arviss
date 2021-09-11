#include "collision_response_system.h"

#include "components/collidables.h"
#include "components/events.h"
#include "entities.h"
#include "raylib.h"
#include "systems/event_system.h"

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
    else if (Entities.Is(id, bmWall))
    {
        return "wall";
    }
    else if (Entities.Is(id, bmDoor))
    {
        return "door";
    }
    return "*** unknown ***";
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
        TraceLog(LOG_INFO, "Collision between %s and %s", Identify(c->firstId), Identify(c->secondId));

        // Don't remove the player, walls or doors.
        if (Entities.AnyOf(c->firstId, bmRobot | bmShot))
        {
            Entities.Set(c->firstId, bmReap);
        }

        // Only remove mobile entities. We don't want to remove walls and doors (that happened).
        if (Entities.AnyOf(c->secondId, bmRobot | bmPlayer | bmShot))
        {
            Entities.Set(c->secondId, bmReap);
        }

        // Handle collisions with the player.
        if (Entities.Is(c->firstId, bmPlayer))
        {
            Collidable* collidable = Collidables.Get(c->secondId);
            if (!collidable->isTrigger)
            {
                // The player has died because they hit something that wasn't a mere trigger.
                Events.Add(&(Event){.type = etPLAYER, .player = (PlayerEvent){.type = peDIED, .id = c->firstId}});
            }
        }
    }
}

void ResetCollisionResponseSystem(void)
{
    EventSystem.Register(HandleEvents);
}

void UpdateCollisionResponseSystem(void)
{
}
