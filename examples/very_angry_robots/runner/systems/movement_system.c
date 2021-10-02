#include "movement_system.h"

#include "entities.h"
#include "systems/event_system.h"
#include "tables/events.h"
#include "tables/positions.h"
#include "tables/steps.h"
#include "tables/velocities.h"

// TODO: this needs to be in _one_ place.
#define NUM_PHYSICS_STEPS 8
#define ALPHA (1.0f / NUM_PHYSICS_STEPS)

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
            if (Entities.Is(id, bmStepped))
            {
                const Step* s = Steps.Get(id);
                if (s->step != 0)
                {
                    continue;
                }
            }
            Position* c = Positions.Get(id);
            Velocity* v = Velocities.Get(id);
            c->position = Vector2Add(c->position, Vector2Scale(v->velocity, ALPHA));
        }
    }
}
