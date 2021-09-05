#pragma once

typedef void (*EventHandler)(int firstEvent, int lastEvent);

void RegisterHandlerWithEventSystem(EventHandler handler);
void ResetEventSystem(void);
void UpdateEventSystem(void);

static struct
{
    void (*Register)(EventHandler handler);
    void (*Reset)(void);
    void (*Update)(void);
} EventSystem = {.Register = RegisterHandlerWithEventSystem, .Reset = ResetEventSystem, .Update = UpdateEventSystem};
