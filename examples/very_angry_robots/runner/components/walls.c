#include "walls.h"

#include "entities.h"

static struct Wall wallComponents[MAX_ENTITIES];

Wall* GetWall(EntityId id)
{
    return &wallComponents[id.id];
}

void SetWall(EntityId id, Wall* wall)
{
    wallComponents[id.id] = *wall;
}

bool IsVerticalWall(EntityId id)
{
    return wallComponents[id.id].vertical;
}
