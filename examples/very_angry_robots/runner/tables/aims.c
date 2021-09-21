#include "aims.h"

#include "entities.h"

static struct Aim aims[MAX_ENTITIES];

Aim* GetAim(EntityId id)
{
    return &aims[id.id];
}

void SetAim(EntityId id, Aim* aim)
{
    aims[id.id] = *aim;
}

Vector2 GetAimValue(EntityId id)
{
    return aims[id.id].aim;
}
