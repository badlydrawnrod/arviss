#include "positions.h"

#include "entities.h"

static struct Position positions[MAX_ENTITIES];

Position* GetPosition(EntityId id)
{
    return &positions[id.id];
}

void SetPosition(EntityId id, Position* position)
{
    positions[id.id] = *position;
}

Vector2 GetPositionValue(EntityId id)
{
    return positions[id.id].position;
}
