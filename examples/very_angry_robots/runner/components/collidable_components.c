#include "collidable_components.h"

#include "entities.h"

static struct CollidableComponent collidableComponents[MAX_ENTITIES];

CollidableComponent* GetCollidableComponent(int id)
{
    return &collidableComponents[id];
}

void SetCollidableComponent(int id, CollidableComponent* collidableComponent)
{
    collidableComponents[id] = *collidableComponent;
}
