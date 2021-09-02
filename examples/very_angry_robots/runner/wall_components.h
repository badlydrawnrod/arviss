#pragma once

#include "entities.h"
#include "raymath.h"

typedef struct WallComponent
{
    bool vertical;
} WallComponent;

WallComponent* GetWallComponent(int id);
void SetWallComponent(int id, WallComponent* wallComponent);
bool IsVerticalWallComponent(int id);

static struct
{
    WallComponent* (*Get)(int id);
    void (*Set)(int id, WallComponent* wallComponent);
    bool (*IsVertical)(int id);
} WallComponents = {.Get = GetWallComponent, .Set = SetWallComponent, .IsVertical = IsVerticalWallComponent};
