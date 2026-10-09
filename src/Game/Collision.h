#pragma once
#include "Aabb.h"

inline bool IntersectAabb(const Aabb& a, const Aabb& b)
{
	if (a.r <= b.l) return false;
	if (a.l >= b.r) return false;
	if (a.b <= b.t) return false;
	if (a.t >= b.b) return false;
	return true;
}