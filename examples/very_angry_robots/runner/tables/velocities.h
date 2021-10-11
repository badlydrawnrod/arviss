#pragma once

#include "entities.h"
#include "raymath.h"

typedef struct Velocity
{
    Vector2 velocity;
} Velocity;

Velocity* GetVelocity(EntityId id);
void SetVelocity(EntityId id, Velocity* velocity);
Vector2 GetVelocityValue(EntityId id);

static struct
{
    Velocity* (*Get)(EntityId id);
    void (*Set)(EntityId id, Velocity* velocity);
    Vector2 (*GetVelocity)(EntityId id);
} Velocities = {.Get = GetVelocity, .Set = SetVelocity, .GetVelocity = GetVelocityValue};
