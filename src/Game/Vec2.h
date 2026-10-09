#pragma once

struct Vec2
{
	float x = 0.0f;
	float y = 0.0f;

	Vec2() = default;
	Vec2(float _x, float _y) : x(_x), y(_y) {}
};