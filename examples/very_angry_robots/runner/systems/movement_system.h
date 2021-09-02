#pragma once

#include "entities.h"
#include "raymath.h"

void UpdateMovementSystem(void);

static struct
{
    void (*Update)(void);
} MovementSystem = {.Update = UpdateMovementSystem};
