#include "queries.h"

EntityId GetQueryPlayerId(void)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        if (Entities.Is((EntityId){i}, bmPlayer))
        {
            return (EntityId){.id = i};
        }
    }
    return (EntityId){.id = -1};
}
