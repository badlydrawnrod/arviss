#pragma once

#include "entities.h"

#include <stdbool.h>

typedef struct Wall
{
    bool vertical;
} Wall;

Wall* GetWall(EntityId id);
void SetWall(EntityId id, Wall* wall);
bool IsVerticalWall(EntityId id);

static struct
{
    Wall* (*Get)(EntityId id);
    void (*Set)(EntityId id, Wall* wall);
    bool (*IsVertical)(EntityId id);
} Walls = {.Get = GetWall, .Set = SetWall, .IsVertical = IsVerticalWall};
