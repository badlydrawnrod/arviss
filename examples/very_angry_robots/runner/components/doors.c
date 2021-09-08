#include "doors.h"

#include "entities.h"

static struct Door doorComponents[MAX_ENTITIES];

Door* GetDoor(EntityId id)
{
    return &doorComponents[id.id];
}

void SetDoor(EntityId id, Door* door)
{
    doorComponents[id.id] = *door;
}

bool IsVerticalDoor(EntityId id)
{
    return doorComponents[id.id].vertical;
}
