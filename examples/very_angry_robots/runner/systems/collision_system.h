#pragma once

void ResetCollisionSystem(void);
void UpdateCollisionSystem(int currentPass);

static struct
{
    void (*Reset)(void);
    void (*Update)(int currentPass);
} CollisionSystem = {.Reset = ResetCollisionSystem, .Update = UpdateCollisionSystem};
