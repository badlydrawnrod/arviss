#pragma once

#include "raymath.h"

typedef struct Position
{
    Vector2 position;
} Position;

Position* GetPosition(int id);
void SetPosition(int id, Position* position);
Vector2 GetPositionValue(int id);

static struct
{
    Position* (*Get)(int id);
    void (*Set)(int id, Position* position);
    Vector2 (*GetPosition)(int id);
} Positions = {.Get = GetPosition, .Set = SetPosition, .GetPosition = GetPositionValue};
