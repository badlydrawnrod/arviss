#pragma once

#include "entities.h"
#include "entrance.h"

#define MAX_EVENTS 1024

typedef enum EventType
{
    etCOLLISION,
    etPLAYER,
    etDOOR
} EventType;

typedef struct CollisionEvent
{
    EntityId firstId;
    EntityId secondId;
} CollisionEvent;

typedef enum PlayerEventType
{
    peDIED,
    peSPAWNED
} PlayerEventType;

typedef struct PlayerEvent
{
    PlayerEventType type;
    EntityId id;
} PlayerEvent;

typedef enum DoorEventType
{
    deEnter,
    deExit
} DoorEventType;

typedef struct DoorEvent
{
    DoorEventType type;
    Entrance entrance; // Which entrance is the door?
} DoorEvent;

typedef struct Event
{
    EventType type;
    union
    {
        CollisionEvent collision;
        PlayerEvent player;
        DoorEvent door;
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
