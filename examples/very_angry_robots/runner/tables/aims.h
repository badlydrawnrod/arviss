#pragma once

#include "entities.h"
#include "raymath.h"

typedef struct Aim
{
    Vector2 aim;
} Aim;

Aim* GetAim(EntityId id);
void SetAim(EntityId id, Aim* aim);
Vector2 GetAimValue(EntityId id);

static struct
{
    Aim* (*Get)(EntityId id);
    void (*Set)(EntityId id, Aim* aim);
    Vector2 (*GetAim)(EntityId id);
} Aims = {.Get = GetAim, .Set = SetAim, .GetAim = GetAimValue};
