#pragma once

#include "raymath.h"

#include <stdbool.h>

typedef struct
{
    Vector2 centre;
    Vector2 extents;
} AABB;

typedef struct
{
    Vector2 origin;
    Vector2 direction; // The direction is not necessarily normalized.
} Ray2D;

static inline float Max(float a, float b)
{
    return a > b ? a : b;
}

static inline float Min(float a, float b)
{
    return a < b ? a : b;
}

static inline Vector2 Vector2Rcp(Vector2 a)
{
    return (Vector2){.x = 1.0f / a.x, .y = 1.0f / a.y};
}

static inline Vector2 Vector2Min(Vector2 a, Vector2 b)
{
    return (Vector2){Min(a.x, b.x), Min(a.y, b.y)};
}

static inline Vector2 Vector2Max(Vector2 a, Vector2 b)
{
    return (Vector2){Max(a.x, b.x), Max(a.y, b.y)};
}

static inline float Vector2MinComponent(Vector2 a)
{
    return Min(a.x, a.y);
}

static inline float Vector2MaxComponent(Vector2 a)
{
    return Max(a.x, a.y);
}

static inline bool Slabs(Vector2 p0, Vector2 p1, Vector2 rayOrigin, Vector2 invRayDir, float* t)
{
    // See: https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525
    // https://gist.githubusercontent.com/bromanz/ed0de6725f5e40a0afd8f50985c2f7ad/raw/be5e79e16181e4617d1a0e6e540dd25c259c76a4/efficient-slab-test-majercik-et-al
    const Vector2 t0 = Vector2Multiply(Vector2Subtract(p0, rayOrigin), invRayDir);
    const Vector2 t1 = Vector2Multiply(Vector2Subtract(p1, rayOrigin), invRayDir);
    const Vector2 tmin = Vector2Min(t0, t1);
    const Vector2 tmax = Vector2Max(t0, t1);
    *t = Max(0.0f, Vector2MaxComponent(tmin));
    return *t <= Min(1.0f, Vector2MinComponent(tmax));
}

bool CheckCollisionRay2dAABBs(Ray2D r, AABB aabb, float* t);
bool CheckCollisionMovingAABBs(AABB a, AABB b, Vector2 va, Vector2 vb, float* t);
