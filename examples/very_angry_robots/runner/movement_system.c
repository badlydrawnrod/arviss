#include "movement_system.h"

#include "dynamic_components.h"

void UpdateMovementSystem(void)
{
    for (int id = 0, numEntities = Entities.Count(); id < numEntities; id++)
    {
        if (Entities.Is(id, bmDynamic))
        {
            DynamicComponent* c = DynamicComponents.Get(id);
            c->position = Vector2Add(c->position, c->movement);
        }
    }
}
