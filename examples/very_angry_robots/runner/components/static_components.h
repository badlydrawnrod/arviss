#pragma once

#include "entities.h"
#include "raymath.h"

typedef struct StaticComponent
{
    Vector2 position;
} StaticComponent;

StaticComponent* GetStaticComponent(int id);
void SetStaticComponent(int id, StaticComponent* staticComponent);
Vector2 GetStaticComponentPosition(int id);

static struct
{
    StaticComponent* (*Get)(int id);
    void (*Set)(int id, StaticComponent* staticComponent);
    Vector2 (*GetPosition)(int id);
} StaticComponents = {.Get = GetStaticComponent, .Set = SetStaticComponent, .GetPosition = GetStaticComponentPosition};
