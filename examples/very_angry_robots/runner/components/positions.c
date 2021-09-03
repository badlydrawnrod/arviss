#include "positions.h"

#include "entities.h"

static struct Position dynamicComponents[MAX_ENTITIES];

Position* GetPosition(int id)
{
    return &dynamicComponents[id];
}

void SetPosition(int id, Position* dynamicComponent)
{
    dynamicComponents[id] = *dynamicComponent;
}

Vector2 GetPositionValue(int id)
{
    return dynamicComponents[id].position;
}
