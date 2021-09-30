#pragma once

#include "entities.h"
#include "raylib.h"
#include "tables/rooms.h"

EntityId MakeShot(Vector2 position, Vector2 aim, EntityId owner);
EntityId MakeRobotShot(Vector2 position, Vector2 aim, EntityId owner);
EntityId MakeRobot(RoomId owner, float x, float y);
EntityId MakePlayer(float x, float y);
EntityId MakeWall(RoomId owner, float x, float y, bool isVertical);
EntityId MakeWallFromGrid(RoomId owner, int gridX, int gridY, bool isVertical);
EntityId MakeExit(RoomId owner, float x, float y, bool isVertical, Entrance entrance);
EntityId MakeExitFromGrid(RoomId owner, int gridX, int gridY, bool isVertical, Entrance entrance);
EntityId MakeDoor(RoomId owner, float x, float y, bool isVertical, Entrance entrance);
EntityId MakeDoorFromGrid(RoomId owner, int gridX, int gridY, bool isVertical, Entrance entrance);
