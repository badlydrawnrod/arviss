#pragma once

#include "entities.h"
#include "raymath.h"

typedef struct DoorComponent
{
    bool vertical;
} DoorComponent;

DoorComponent* GetDoorComponent(int id);
void SetDoorComponent(int id, DoorComponent* doorComponent);
bool IsVerticalDoorComponent(int id);

static struct
{
    DoorComponent* (*Get)(int id);
    void (*Set)(int id, DoorComponent* doorComponent);
    bool (*IsVertical)(int id);
} DoorComponents = {.Get = GetDoorComponent, .Set = SetDoorComponent, .IsVertical = IsVerticalDoorComponent};
