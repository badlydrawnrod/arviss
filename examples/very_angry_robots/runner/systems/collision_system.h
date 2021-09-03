#pragma once

void UpdateCollisionSystem(void);

static struct
{
    void (*Update)(void);
} CollisionSystem = {.Update = UpdateCollisionSystem};
