#include "collidable_components.h"

#include "entities.h"

static struct CollidableComponent collidableComponents[MAX_ENTITIES];

CollidableComponent* GetCollidableComponent(EntityId id)
{
    return &collidableComponents[id.id];
}

void SetCollidableComponent(EntityId id, CollidableComponent* collidableComponent)
{
    collidableComponents[id.id] = *collidableComponent;
}
