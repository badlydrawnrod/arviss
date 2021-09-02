#include "wall_components.h"

#include "entities.h"

static struct WallComponent wallComponents[MAX_ENTITIES];

WallComponent* GetWallComponent(int id)
{
    return &wallComponents[id];
}

void SetWallComponent(int id, WallComponent* wallComponent)
{
    wallComponents[id] = *wallComponent;
}

bool IsVerticalWallComponent(int id)
{
    return wallComponents[id].vertical;
}
