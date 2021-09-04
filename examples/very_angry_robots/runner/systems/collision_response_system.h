#pragma once

void UpdateCollisionResponseSystem(void);

static struct
{
    void (*Update)(void);
} CollisionResponseSystem = {.Update = UpdateCollisionResponseSystem};
