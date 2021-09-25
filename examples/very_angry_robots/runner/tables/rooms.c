#include "rooms.h"

#include "entities.h"

static struct Room rooms[MAX_ENTITIES];

Room* GetRoom(EntityId id)
{
    return &rooms[id.id];
}

void SetRoom(EntityId id, Room* room)
{
    rooms[id.id] = *room;
}
