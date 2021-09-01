#pragma once

#include "entities.h"

void UpdatePlayerActions(void);

static struct
{
    void (*Update)(void);
} PlayerActionSystem = {.Update = UpdatePlayerActions};
