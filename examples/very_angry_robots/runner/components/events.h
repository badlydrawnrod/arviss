#pragma once

#define MAX_EVENTS 1024

typedef enum EventType
{
    etCOLLISION
} EventType;

typedef struct CollisionEvent
{
    int firstId;
    int secondId;
} CollisionEvent;

typedef struct DeathEvent
{
    int id;
} DeathEvent;

typedef struct Event
{
    EventType type;
    union
    {
        CollisionEvent collision;
        DeathEvent death;
    };
} Event;

typedef struct EventId
{
    int id;
} EventId;

void ClearEvents(void);
int CountEvents(void);
Event* GetEvent(EventId id);
EventId AddEvent(Event* event);

static struct
{
    void (*Clear)(void);
    int (*Count)(void);
    Event* (*Get)(EventId id);
    EventId (*Add)(Event* event);
} Events = {.Clear = ClearEvents, .Count = CountEvents, .Get = GetEvent, .Add = AddEvent};
