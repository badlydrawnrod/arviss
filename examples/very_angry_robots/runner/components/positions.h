#pragma once

#include "entities.h"
#include "raymath.h"

typedef struct Position
{
    Vector2 position;
} Position;

Position* GetPosition(EntityId id);
void SetPosition(EntityId id, Position* position);
Vector2 GetPositionValue(EntityId id);

static struct
{
    Position* (*Get)(EntityId id);
    void (*Set)(EntityId id, Position* position);
    Vector2 (*GetPosition)(EntityId id);
} Positions = {.Get = GetPosition, .Set = SetPosition, .GetPosition = GetPositionValue};
