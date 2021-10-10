#include "guests.h"

#define MAX_GUESTS 64

static GuestId guestsByEntity[MAX_ENTITIES];
static Guest guests[MAX_GUESTS];

void ClearGuests(void)
{
    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        guestsByEntity[i].id = -1;
    }
}

Guest* GetGuest(EntityId id)
{
    // TODO: sanity check.
    GuestId guestId = guestsByEntity[id.id];
    return &guests[guestId.id];
}

GuestId MakeGuest(EntityId id)
{
    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        if (guestsByEntity[i].id == -1)
        {
            guestsByEntity[i].id = i;
            return guestsByEntity[i];
        }
    }
    return (GuestId){.id = -1};
}
