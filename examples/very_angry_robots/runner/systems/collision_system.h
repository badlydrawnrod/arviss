#pragma once

void ResetCollisionSystem(void);
void UpdateCollisionSystem(void);

static struct
{
    void (*Reset)(void);
    void (*Update)(void);
} CollisionSystem = {.Reset = ResetCollisionSystem, .Update = UpdateCollisionSystem};
