#include "geometry.h"

bool CheckCollisionRay2dAABBs(Ray2D r, AABB aabb, float* t)
{
    const Vector2 invD = Vector2Rcp(r.direction);
    const Vector2 aabbMin = Vector2Subtract(aabb.centre, aabb.extents);
    const Vector2 aabbMax = Vector2Add(aabb.centre, aabb.extents);
    return Slabs(aabbMin, aabbMax, r.origin, invD, t);
}

bool CheckCollisionMovingAABBs(AABB a, AABB b, Vector2 va, Vector2 vb, float* t)
{
    // An AABB at B's position with the combined size of A and B.
    const AABB aabb = {.centre = b.centre, .extents = Vector2Add(a.extents, b.extents)};

    // A ray at A's position with its direction set to A's velocity relative to B. It's a parametric representation of a
    // line representing A's position at time t, where 0 <= t <= 1.
    const Ray2D r = {.origin = a.centre, .direction = Vector2Subtract(va, vb)};

    // Does the ray hit the AABB
    return CheckCollisionRay2dAABBs(r, aabb, t);
}
