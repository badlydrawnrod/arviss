#pragma once

#include "entities.h"
#include "types.h"

#include <stdbool.h>

typedef struct Room
{
    RoomId roomId; // Which room owns this entity?
} Room;

Room* GetOwner(EntityId id);
void SetOwner(EntityId id, Room* owner);

static struct
{
    Room* (*Get)(EntityId id);
    void (*Set)(EntityId id, Room* owner);
} Owners = {.Get = GetOwner, .Set = SetOwner};
