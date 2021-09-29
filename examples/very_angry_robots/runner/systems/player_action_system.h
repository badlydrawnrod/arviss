#pragma once

#include "entities.h"

void ResetPlayerActions(void);
void UpdatePlayerActions(void);
void HandleTriggersPlayerActions(void);

static struct
{
    void (*Reset)(void);
    void (*Update)(void);
    void (*HandleTriggers)(void);
} PlayerActionSystem = {.Reset = ResetPlayerActions, .Update = UpdatePlayerActions, .HandleTriggers = HandleTriggersPlayerActions};
