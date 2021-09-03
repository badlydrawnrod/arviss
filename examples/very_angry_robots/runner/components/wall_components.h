#pragma once

#include "entities.h"

#include <stdbool.h>

typedef struct WallComponent
{
    bool vertical;
} WallComponent;

WallComponent* GetWallComponent(EntityId id);
void SetWallComponent(EntityId id, WallComponent* wallComponent);
bool IsVerticalWallComponent(EntityId id);

static struct
{
    WallComponent* (*Get)(EntityId id);
    void (*Set)(EntityId id, WallComponent* wallComponent);
    bool (*IsVertical)(EntityId id);
} WallComponents = {.Get = GetWallComponent, .Set = SetWallComponent, .IsVertical = IsVerticalWallComponent};
