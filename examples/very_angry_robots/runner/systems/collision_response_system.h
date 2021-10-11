#pragma once

void ResetCollisionResponseSystem(void);
void UpdateCollisionResponseSystem(void);

static struct
{
    void (*Reset)(void);
    void (*Update)(void);
} CollisionResponseSystem = {.Reset = ResetCollisionResponseSystem, .Update = UpdateCollisionResponseSystem};
