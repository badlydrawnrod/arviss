#pragma once

#include "entities.h"

typedef enum CollidableType
{
    ctROBOT,
    ctPLAYER,
    ctHWALL,
    ctVWALL,
    ctHDOOR,
    ctVDOOR
} CollidableType;

typedef struct CollidableComponent
{
    CollidableType type;
} CollidableComponent;

CollidableComponent* GetCollidableComponent(EntityId id);
void SetCollidableComponent(EntityId id, CollidableComponent* collidableComponent);

static struct
{
    CollidableComponent* (*Get)(EntityId id);
    void (*Set)(EntityId id, CollidableComponent* collidableComponent);
} CollidableComponents = {.Get = GetCollidableComponent, .Set = SetCollidableComponent};
