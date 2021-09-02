#include "door_components.h"

static struct DoorComponent doorComponents[MAX_ENTITIES];

DoorComponent* GetDoorComponent(int id)
{
    return &doorComponents[id];
}

void SetDoorComponent(int id, DoorComponent* doorComponent)
{
    doorComponents[id] = *doorComponent;
}

bool IsVerticalDoorComponent(int id)
{
    return doorComponents[id].vertical;
}
