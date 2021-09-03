#include "wall_components.h"

#include "entities.h"

static struct WallComponent wallComponents[MAX_ENTITIES];

WallComponent* GetWallComponent(EntityId id)
{
    return &wallComponents[id.id];
}

void SetWallComponent(EntityId id, WallComponent* wallComponent)
{
    wallComponents[id.id] = *wallComponent;
}

bool IsVerticalWallComponent(EntityId id)
{
    return wallComponents[id.id].vertical;
}
