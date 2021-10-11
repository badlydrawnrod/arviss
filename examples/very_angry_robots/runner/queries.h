#pragma once

#include "entities.h"

EntityId GetQueryPlayerId(void);

static struct
{
    EntityId (*GetPlayerId)(void);
} Queries = {.GetPlayerId = GetQueryPlayerId};
