#pragma once

#include "entities.h"
#include "types.h"

#include <stdbool.h>

typedef struct Owner
{
    EntityId ownerId; // Which entity is the owner of this one?
} Owner;

Owner* GetOwner(EntityId id);
void SetOwner(EntityId id, Owner* owner);

static struct
{
    Owner* (*Get)(EntityId id);
    void (*Set)(EntityId id, Owner* owner);
} Owners = {.Get = GetOwner, .Set = SetOwner};
