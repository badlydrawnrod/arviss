#include "door_components.h"

#include "entities.h"

static struct DoorComponent doorComponents[MAX_ENTITIES];

DoorComponent* GetDoorComponent(EntityId id)
{
    return &doorComponents[id.id];
}

void SetDoorComponent(EntityId id, DoorComponent* doorComponent)
{
    doorComponents[id.id] = *doorComponent;
}

bool IsVerticalDoorComponent(EntityId id)
{
    return doorComponents[id.id].vertical;
}
