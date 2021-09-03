#include "velocities.h"

#include "entities.h"

static struct Velocity velocities[MAX_ENTITIES];

Velocity* GetVelocity(int id)
{
    return &velocities[id];
}

void SetVelocity(int id, Velocity* velocity)
{
    velocities[id] = *velocity;
}

Vector2 GetVelocityValue(int id)
{
    return velocities[id].velocity;
}
