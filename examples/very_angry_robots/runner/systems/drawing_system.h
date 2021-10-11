#pragma once

void ResetDrawingSystem(void);
void UpdateDrawingSystem(void);

static struct
{
    void (*Reset)(void);
    void (*Update)(void);
} DrawingSystem = {.Reset = ResetDrawingSystem, .Update = UpdateDrawingSystem};
