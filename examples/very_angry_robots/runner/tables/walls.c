#include "walls.h"

#include "entities.h"

static struct Wall walls[MAX_ENTITIES];

Wall* GetWall(EntityId id)
{
    return &walls[id.id];
}

void SetWall(EntityId id, Wall* wall)
{
    walls[id.id] = *wall;
}

bool IsVerticalWall(EntityId id)
{
    return walls[id.id].vertical;
}
