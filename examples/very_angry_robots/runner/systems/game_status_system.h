#pragma once

#include "entities.h"

bool IsGameOverGameStatusSystem(void);
void ResetGameStatusSystem(void);
void UpdateGameStatusSystem(void);

static struct
{
    bool (*IsGameOver)(void);
    void (*Reset)(void);
    void (*Update)(void);
} GameStatusSystem = {.IsGameOver = IsGameOverGameStatusSystem, .Reset = ResetGameStatusSystem, .Update = UpdateGameStatusSystem};
