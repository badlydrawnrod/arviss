#pragma once

#include "arviss.h"
#include "entities.h"
#include "loadelf.h"
#include "mem.h"
#include "types.h"

#include <stdbool.h>

typedef struct GuestId
{
    int id;
} GuestId;

typedef struct Guest
{
    ArvissCpu cpu;
    Memory memory;
} Guest;

void ClearGuests(void);
Guest* GetGuest(EntityId id);
GuestId MakeGuest(EntityId id);

static struct
{
    void (*Clear)(void);
    Guest* (*Get)(EntityId id);
    GuestId (*Make)(EntityId id);
} Guests = {.Clear = ClearGuests, .Get = GetGuest, .Make = MakeGuest};
