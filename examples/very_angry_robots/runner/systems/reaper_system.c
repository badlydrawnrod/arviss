#include "reaper_system.h"

#include "entities.h"
#include "tables/guests.h"

void UpdateReaperSystem(void)
{
    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmReap))
        {
            // This is somewhat specific, but pass the salt. https://xkcd.com/974/
            if (Entities.Is(id, bmGuest))
            {
                Entities.Clear(id, bmGuest);
                Guests.Free(id);
            }
            Entities.Destroy(id);
        }
    }
}
