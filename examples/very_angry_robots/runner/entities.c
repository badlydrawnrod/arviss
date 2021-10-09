#include "entities.h"

#include "raylib.h"

static int maxCount = 0;
static int count = 0;

static Entity entities[MAX_ENTITIES];

void ResetEntities(void)
{
    maxCount = 0;
    count = 0;
    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        entities[i].bitmap = 0;
    }
}

int CountEntity(void)
{
    return count;
}

int MaxCountEntity(void)
{
    return maxCount;
}

int CreateEntity(void)
{
    for (int id = 0; id < MAX_ENTITIES; id++)
    {
        if (entities[id].bitmap == 0)
        {
            ++count;
            if (id == maxCount)
            {
                ++maxCount;
            }
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
        --count;
        if (id.id == maxCount - 1)
        {
            --maxCount;
        }
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
