#pragma once

#include "raymath.h"

typedef struct Velocity
{
    Vector2 velocity;
} Velocity;

Velocity* GetVelocity(int id);
void SetVelocity(int id, Velocity* velocity);
Vector2 GetVelocityValue(int id);

static struct
{
    Velocity* (*Get)(int id);
    void (*Set)(int id, Velocity* velocity);
    Vector2 (*GetVelocity)(int id);
} Velocities = {.Get = GetVelocity, .Set = SetVelocity, .GetVelocity = GetVelocityValue};
