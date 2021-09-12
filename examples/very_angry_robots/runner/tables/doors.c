#include "doors.h"

#include "entities.h"

static struct Door doors[MAX_ENTITIES];

Door* GetDoor(EntityId id)
{
    return &doors[id.id];
}

void SetDoor(EntityId id, Door* door)
{
    doors[id.id] = *door;
}

bool IsVerticalDoor(EntityId id)
{
    return doors[id.id].vertical;
}
