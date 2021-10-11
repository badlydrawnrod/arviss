#include "events.h"

static int numEvents = 0;
static struct Event events[MAX_EVENTS];

void ClearEvents(void)
{
    numEvents = 0;
}

int CountEvents(void)
{
    return numEvents;
}

Event* GetEvent(EventId id)
{
    return &events[id.id];
}

EventId AddEvent(Event* event)
{
    EventId id = {.id = numEvents};
    events[numEvents] = *event;
    ++numEvents;
    return id;
}
