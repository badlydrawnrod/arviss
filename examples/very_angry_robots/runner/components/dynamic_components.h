#pragma once

#include "raymath.h"

typedef struct DynamicComponent
{
    Vector2 position;
    Vector2 movement;
} DynamicComponent;

DynamicComponent* GetDynamicComponent(int id);
void SetDynamicComponent(int id, DynamicComponent* dynamicComponent);
Vector2 GetDynamicComponentPosition(int id);

static struct
{
    DynamicComponent* (*Get)(int id);
    void (*Set)(int id, DynamicComponent* dynamicComponent);
    Vector2 (*GetPosition)(int id);
} DynamicComponents = {.Get = GetDynamicComponent, .Set = SetDynamicComponent, .GetPosition = GetDynamicComponentPosition};
