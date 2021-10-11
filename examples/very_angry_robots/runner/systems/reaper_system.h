#pragma once

void UpdateReaperSystem(void);

static struct
{
    void (*Update)(void);
} ReaperSystem = {.Update = UpdateReaperSystem};
