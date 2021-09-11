#pragma once

void ResetMovementSystem(void);
void UpdateMovementSystem(void);

static struct
{
    void (*Reset)(void);
    void (*Update)(void);
} MovementSystem = {.Reset = ResetMovementSystem, .Update = UpdateMovementSystem};
