#pragma once

#include "entities.h"

void ResetGameStatusSystem(void);
void UpdateGameStatusSystem(void);

static struct
{
    void (*Reset)(void);
    void (*Update)(void);
} GameStatusSystem = {.Reset = ResetGameStatusSystem, .Update = UpdateGameStatusSystem};
