#include "entities.h"

static int numEntities = 0; // TODO: rename to maxAssignedEntity and update when necessary.
static Entity entities[MAX_ENTITIES];

void ResetEntities(void)
{
    numEntities = 0;
    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        entities[i].bitmap = 0;
    }
}

// TODO: rename this to MaxAssignedEntity.
int CountEntities(void)
{
    return numEntities;
}

int CreateEntity(void)
{
    for (int id = 0; id < MAX_ENTITIES; id++)
    {
        if (entities[id].bitmap == 0)
        {
            ++numEntities;
            return id;
        }
    }
    return -1;
}

void DestroyEntity(EntityId id)
{
    if (entities[id.id].bitmap != 0)
    {
        entities[id.id].bitmap = 0;
        --numEntities;
    }
}

bool IsEntity(EntityId id, Component mask)
{
    return (entities[id.id].bitmap & mask) == mask;
}

bool AnyOfEntity(EntityId id, Component mask)
{
    return (entities[id.id].bitmap & mask) != 0;
}

void ClearEntity(EntityId id, Component mask)
{
    entities[id.id].bitmap &= ~mask;
}

void SetEntity(EntityId id, Component mask)
{
    entities[id.id].bitmap |= mask;
}
