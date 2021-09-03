#include "event_system.h"

#include "components/events.h"
#include "raylib.h"

void UpdateEventSystem(void)
{
    for (int i = 0, numEvents = Events.Count(); i < numEvents; i++)
    {
        Event* e = Events.Get((EventId){.id = i});
        if (e->type == etCOLLISION)
        {
            TraceLog(LOG_INFO, "Collision between %d and %d", e->collision.firstId, e->collision.secondId);
        }
    }
    Events.Clear();
}
