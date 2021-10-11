#pragma once

#include "entities.h"
#include "types.h"

#include <stdbool.h>

typedef struct Room
{
    RoomId roomId; // Which room owns this entity?
} Room;

Room* GetRoom(EntityId id);
void SetRoom(EntityId id, Room* room);

static struct
{
    Room* (*Get)(EntityId id);
    void (*Set)(EntityId id, Room* room);
} Rooms = {.Get = GetRoom, .Set = SetRoom};
