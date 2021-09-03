#include "movement_system.h"

#include "components/positions.h"
#include "components/velocities.h"
#include "entities.h"

void UpdateMovementSystem(void)
{
    for (int id = 0, numEntities = Entities.Count(); id < numEntities; id++)
    {
        if (Entities.Is(id, bmPosition | bmVelocity))
        {
            Position* c = Positions.Get(id);
            Velocity* v = Velocities.Get(id);
            c->position = Vector2Add(c->position, v->velocity);
        }
    }
}
