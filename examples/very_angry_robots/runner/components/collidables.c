#include "collidables.h"

#include "entities.h"

static struct Collidable collidables[MAX_ENTITIES];

Collidable* GetCollidable(EntityId id)
{
    return &collidables[id.id];
}

void SetCollidable(EntityId id, Collidable* collidable)
{
    collidables[id.id] = *collidable;
}
