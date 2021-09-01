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

void DestroyEntity(int id)
{
    if (entities[id].bitmap != 0)
    {
        entities[id].bitmap = 0;
        --numEntities;
    }
}

bool IsEntity(int id, Component mask)
{
    return (entities[id].bitmap & mask) == mask;
}

void ClearEntity(int id, Component mask)
{
    entities[id].bitmap &= ~mask;
}

void SetEntity(int id, Component mask)
{
    entities[id].bitmap |= mask;
}
