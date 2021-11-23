#pragma once

#include <utility>
#include <string>
#include "Math.h"

struct Freepoint
{
	Freepoint() = default;
	Freepoint(std::string name, const vec3& pos)
		: name(std::move(name)),
		pos(pos)
	{
	}

	std::string name;
	vec3 pos;
	float dX = 0;
	float dZ = 0;
};
