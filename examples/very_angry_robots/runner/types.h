#pragma once

typedef enum Entrance
{
    fromTOP,
    fromBOTTOM,
    fromLEFT,
    fromRIGHT
} Entrance;

typedef int RoomId;

typedef double GameTime; // More for refactoring convenience than type safety.
