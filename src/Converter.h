#pragma once

#include "Waypoint.h"
#include <string_view>
#include <vector>
#include <map>

class Converter
{
public:
	void read(std::string_view fileName);
	void write(std::string_view fileName) const;

private:
	std::vector<std::string> split(const std::string& line) const;

	std::map<int, std::shared_ptr<Waypoint>> waypoints;
};
