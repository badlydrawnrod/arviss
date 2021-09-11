#include "movement_system.h"

#include "components/events.h"
#include "components/positions.h"
#include "components/velocities.h"
#include "entities.h"
#include "systems/event_system.h"

static bool isEnabled = true;

static void HandleEvents(int first, int last)
{
    for (int i = first; i != last; i++)
    {
        const Event* e = Events.Get((EventId){.id = i});
        if (e->type == etDOOR)
        {
            DoorEvent* de = &e->door;
            isEnabled = de->type == deENTER;
        }
    }
}

void ResetMovementSystem(void)
{
    isEnabled = true;
    EventSystem.Register(HandleEvents);
}

void UpdateMovementSystem(void)
{
    if (!isEnabled)
    {
        return;
    }

    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmPosition | bmVelocity))
        {
            Position* c = Positions.Get(id);
            Velocity* v = Velocities.Get(id);
            c->position = Vector2Add(c->position, v->velocity);
        }
    }
}
