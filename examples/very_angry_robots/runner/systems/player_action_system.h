#pragma once

#include "entities.h"

void ResetPlayerActions(void);
void UpdatePlayerActions(void);

static struct
{
    void (*Reset)(void);
    void (*Update)(void);
} PlayerActionSystem = {.Reset = ResetPlayerActions, .Update = UpdatePlayerActions};
