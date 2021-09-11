#pragma once

#include "entities.h"

#include <stdbool.h>

typedef enum CollidableType
{
    ctROBOT,
    ctPLAYER,
    ctHWALL,
    ctVWALL,
    ctHDOOR,
    ctVDOOR
} CollidableType;

typedef struct Collidable
{
    CollidableType type;
    bool isTrigger;
} Collidable;

Collidable* GetCollidable(EntityId id);
void SetCollidable(EntityId id, Collidable* collidable);

static struct
{
    Collidable* (*Get)(EntityId id);
    void (*Set)(EntityId id, Collidable* collidable);
} Collidables = {.Get = GetCollidable, .Set = SetCollidable};
