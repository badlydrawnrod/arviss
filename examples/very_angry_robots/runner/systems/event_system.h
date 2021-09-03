#pragma once

void UpdateEventSystem(void);

static struct
{
    void (*Update)(void);
} EventSystem = {.Update = UpdateEventSystem};
