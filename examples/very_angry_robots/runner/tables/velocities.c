#include "velocities.h"

#include "entities.h"

static struct Velocity velocities[MAX_ENTITIES];

Velocity* GetVelocity(EntityId id)
{
    return &velocities[id.id];
}

void SetVelocity(EntityId id, Velocity* velocity)
{
    velocities[id.id] = *velocity;
}

Vector2 GetVelocityValue(EntityId id)
{
    return velocities[id.id].velocity;
}
