#include "event_system.h"

#include "components/events.h"
#include "entities.h"
#include "raylib.h"

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

void UpdateEventSystem(void)
{
    for (int i = 0, numEvents = Events.Count(); i < numEvents; i++)
    {
        const Event* e = Events.Get((EventId){.id = i});
        if (e->type == etCOLLISION)
        {
            const CollisionEvent* c = &e->collision;
            TraceLog(LOG_INFO, "Collision between %s and %s", Identify(c->firstId), Identify(c->secondId));
        }
    }
    Events.Clear();
}
