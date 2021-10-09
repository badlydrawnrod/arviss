#pragma once

#include "entities.h"
#include "geometry.h"

#include <stdbool.h>

typedef enum CollidableType
{
    ctROBOT,
    ctPLAYER,
    ctSHOT,
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
AABB GetCollidableGeometry(EntityId id);

static struct
{
    Collidable* (*Get)(EntityId id);
    void (*Set)(EntityId id, Collidable* collidable);
    AABB (*GetGeometry)(EntityId);
} Collidables = {.Get = GetCollidable, .Set = SetCollidable, .GetGeometry = GetCollidableGeometry};
