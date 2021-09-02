#include "movement_system.h"

#include "components/dynamic_components.h"
#include "entities.h"

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
