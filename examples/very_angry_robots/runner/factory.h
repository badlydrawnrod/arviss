#pragma once

#include "entities.h"
#include "raylib.h"
#include "tables/rooms.h"

EntityId MakeShot(RoomId roomId, Vector2 position, Vector2 aim, EntityId owner);
EntityId MakeRobotShot(RoomId roomId, Vector2 position, Vector2 aim, EntityId owner);
EntityId MakeRobot(RoomId roomId, float x, float y);
EntityId MakePlayer(float x, float y);
EntityId MakeWall(RoomId roomId, float x, float y, bool isVertical);
EntityId MakeWallFromGrid(RoomId roomId, int gridX, int gridY, bool isVertical);
EntityId MakeExit(RoomId roomId, float x, float y, bool isVertical, Entrance entrance);
EntityId MakeExitFromGrid(RoomId roomId, int gridX, int gridY, bool isVertical, Entrance entrance);
EntityId MakeDoor(RoomId roomId, float x, float y, bool isVertical, Entrance entrance);
EntityId MakeDoorFromGrid(RoomId roomId, int gridX, int gridY, bool isVertical, Entrance entrance);
