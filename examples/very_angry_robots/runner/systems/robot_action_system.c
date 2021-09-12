#include "robot_action_system.h"

#include "systems/event_system.h"
#include "tables/events.h"

static bool isEnabled = true;

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

void UpdateRobotActions(void)
{
    if (!isEnabled)
    {
        return;
    }
}
