#include "positions.h"

#include "entities.h"

static struct Position positions[MAX_ENTITIES];

Position* GetPosition(int id)
{
    return &positions[id];
}

void SetPosition(int id, Position* position)
{
    positions[id] = *position;
}

Vector2 GetPositionValue(int id)
{
    return positions[id].position;
}
