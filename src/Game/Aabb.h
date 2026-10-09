#pragma once

struct Aabb
{
    float l = 0.0f;
    float t = 0.0f;
    float r = 0.0f;
    float b = 0.0f;
};

inline bool IsHitAabb(const Aabb& a, const Aabb& b)
{
    if (a.r < b.l) return false;
    if (a.l > b.r) return false;
    if (a.b < b.t) return false;
    if (a.t > b.b) return false;

    return true;
}