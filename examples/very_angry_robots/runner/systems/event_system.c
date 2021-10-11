#include "event_system.h"

#include "raylib.h"
#include "tables/events.h"

#define MAX_HANDLERS 32

static EventHandler handlers[MAX_HANDLERS];
static int numHandlers = 0;

void RegisterHandlerWithEventSystem(EventHandler handler)
{
    handlers[numHandlers] = handler;
    ++numHandlers;
}

void ResetEventSystem(void)
{
    numHandlers = 0;
}

void UpdateEventSystem(void)
{
    int lastEvent = 0;

    while (lastEvent != Events.Count())
    {
        int firstEvent = lastEvent;
        lastEvent = Events.Count();
        for (int i = 0; i < numHandlers; i++)
        {
            handlers[i](firstEvent, lastEvent);
        }
    }

    Events.Clear();
}
