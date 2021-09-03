#include "reaper_system.h"

#include "entities.h"

void UpdateReaperSystem(void)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmReap))
        {
            Entities.Destroy(id);
        }
    }
}
