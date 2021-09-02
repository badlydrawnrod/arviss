#pragma once

void UpdateMovementSystem(void);

static struct
{
    void (*Update)(void);
} MovementSystem = {.Update = UpdateMovementSystem};
