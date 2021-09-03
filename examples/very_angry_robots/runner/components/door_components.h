#pragma once

#include "entities.h"

#include <stdbool.h>

typedef struct DoorComponent
{
    bool vertical;
} DoorComponent;

DoorComponent* GetDoorComponent(EntityId id);
void SetDoorComponent(EntityId id, DoorComponent* doorComponent);
bool IsVerticalDoorComponent(EntityId id);

static struct
{
    DoorComponent* (*Get)(EntityId id);
    void (*Set)(EntityId id, DoorComponent* doorComponent);
    bool (*IsVertical)(EntityId id);
} DoorComponents = {.Get = GetDoorComponent, .Set = SetDoorComponent, .IsVertical = IsVerticalDoorComponent};
