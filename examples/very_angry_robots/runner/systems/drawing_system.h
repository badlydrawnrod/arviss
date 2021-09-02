#pragma once

void UpdateDrawingSystem(void);

static struct
{
    void (*Update)(void);
} DrawingSystem = {.Update = UpdateDrawingSystem};
