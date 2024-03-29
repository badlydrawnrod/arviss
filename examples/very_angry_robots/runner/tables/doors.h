#pragma once

#include "entities.h"
#include "types.h"

#include <stdbool.h>

typedef struct Door
{
    bool vertical;
    Entrance leadsTo; // This door leads to an entrance from the given direction.
} Door;

Door* GetDoor(EntityId id);
void SetDoor(EntityId id, Door* door);
bool IsVerticalDoor(EntityId id);

static struct
{
    Door* (*Get)(EntityId id);
    void (*Set)(EntityId id, Door* door);
    bool (*IsVertical)(EntityId id);
} Doors = {.Get = GetDoor, .Set = SetDoor, .IsVertical = IsVerticalDoor};
