#include "guests.h"

#include "raylib.h"

#include <string.h>

#define MAX_GUESTS 64

static GuestId guestsByEntity[MAX_ENTITIES];
static struct
{
    bool allocated;
    Guest guest;
} guests[MAX_GUESTS];

static const uint32_t membase = MEMBASE;
static const uint32_t memsize = MEMSIZE;
static const uint32_t rambase = RAMBASE;
static const uint32_t ramsize = RAMSIZE;

static uint8_t Read8(BusToken token, uint32_t addr, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= membase && addr < membase + memsize)
    {
        return memory->mem[addr - membase];
    }

    *busCode = bcLOAD_ACCESS_FAULT;
    return 0;
}

static uint16_t Read16(BusToken token, uint32_t addr, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= membase && addr < membase + memsize - 1)
    {
        // TODO: implement for big-endian ISAs.
        const uint16_t* base = (uint16_t*)&memory->mem[addr - membase];
        return *base;
    }

    *busCode = bcLOAD_ACCESS_FAULT;
    return 0;
}

static uint32_t Read32(BusToken token, uint32_t addr, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= membase && addr < membase + memsize - 3)
    {
        // TODO: implement for big-endian ISAs.
        const uint32_t* base = (uint32_t*)&memory->mem[addr - membase];
        return *base;
    }

    *busCode = bcLOAD_ACCESS_FAULT;
    return 0;
}

static void Write8(BusToken token, uint32_t addr, uint8_t byte, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= rambase && addr < rambase + ramsize)
    {
        memory->mem[addr - membase] = byte;
        return;
    }
    *busCode = bcSTORE_ACCESS_FAULT;
}

static void Write16(BusToken token, uint32_t addr, uint16_t halfword, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= rambase && addr < rambase + ramsize - 1)
    {
        // TODO: implement for big-endian ISAs.
        uint16_t* base = (uint16_t*)&memory->mem[addr - membase];
        *base = halfword;
        return;
    }

    *busCode = bcSTORE_ACCESS_FAULT;
}

static void Write32(BusToken token, uint32_t addr, uint32_t word, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= rambase && addr < rambase + ramsize - 2)
    {
        // TODO: implement for big-endian ISAs.
        uint32_t* base = (uint32_t*)&memory->mem[addr - membase];
        *base = word;
        return;
    }

    *busCode = bcSTORE_ACCESS_FAULT;
}

static void ZeroMem(ElfToken token, uint32_t addr, uint32_t len)
{
    uint8_t* target = token.t;
    memset(target + addr, 0, len);
}

static void WriteV(ElfToken token, uint32_t addr, void* src, uint32_t len)
{
    uint8_t* target = token.t;
    memcpy(target + addr, src, len);
}

static void Init(Guest* guest)
{
    ArvissInit(&guest->cpu,
               &(Bus){.token = {&guest->memory},
                      .Read8 = Read8,
                      .Read16 = Read16,
                      .Read32 = Read32,
                      .Write8 = Write8,
                      .Write16 = Write16,
                      .Write32 = Write32});

    const char* filename = "../../../../examples/very_angry_robots/arviss/bin/robot";
    if (LoadElf(filename,
                &(ElfLoaderConfig){.token = {&guest->memory.mem},
                                   .zeroMemFn = ZeroMem,
                                   .writeMemFn = WriteV,
                                   .targetSegments = (ElfSegmentDescriptor[]){{.start = ROM_START, .size = ROMSIZE},
                                                                              {.start = RAMBASE, .size = RAMSIZE}},
                                   .numSegments = 2}))
    {
        TraceLog(LOG_WARNING, "--- Failed to load %s", filename);
    }
}

void ClearGuests(void)
{
    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        guestsByEntity[i].id = -1;
    }
    for (int i = 0; i < MAX_GUESTS; i++)
    {
        guests[i].allocated = false;
    }
}

void FreeGuest(EntityId id)
{
    GuestId guestId = guestsByEntity[id.id];
    guests[guestId.id].allocated = false;
    guestsByEntity[id.id].id = -1;
}

Guest* GetGuest(EntityId id)
{
    // TODO: sanity check.
    GuestId guestId = guestsByEntity[id.id];
    return &guests[guestId.id].guest;
}

GuestId MakeGuest(EntityId id)
{
    for (int i = 0; i < MAX_GUESTS; i++)
    {
        if (!guests[i].allocated)
        {
            guestsByEntity[id.id].id = i;
            guests[i].allocated = true;
            Init(&guests[i].guest);
            return guestsByEntity[i];
        }
    }
    return (GuestId){.id = -1};
}
