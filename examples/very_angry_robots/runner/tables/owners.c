#include "owners.h"

#include "entities.h"

static struct Owner owners[MAX_ENTITIES];

Owner* GetOwner(EntityId id)
{
    return &owners[id.id];
}

void SetOwner(EntityId id, Owner* owner)
{
    owners[id.id] = *owner;
}
