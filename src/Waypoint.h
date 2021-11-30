#pragma once

#include <utility>
#include <string>
#include <vector>
#include <memory>
#include "Math.h"

struct Waypoint
{
	std::string name;
	vec3 pos;
	float dX = 0;
	float dZ = 0;
	std::vector<std::string> connections;
};

struct Way
{
	int objectId;
	std::shared_ptr<Waypoint> waypoint;
};
