#include "rooms.h"

#include "entities.h"

static struct Room owners[MAX_ENTITIES];

Room* GetOwner(EntityId id)
{
    return &owners[id.id];
}

void SetOwner(EntityId id, Room* owner)
{
    owners[id.id] = *owner;
}
