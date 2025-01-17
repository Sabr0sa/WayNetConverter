#pragma once

#include "Waypoint.h"
#include "Freepoint.h"
#include <string_view>
#include <vector>
#include <map>

class Converter
{
public:
	void readZen(std::string_view fileName);
	void writeWp(std::string_view fileName) const;
	void writeFp(std::string_view fileName) const;

private:
	static std::vector<std::string> split(const std::string& line);
	static bool hexstr2float(const char *str, float& ret);

	std::map<int, std::shared_ptr<Waypoint>> waypoints;
	std::vector<Freepoint> freepoints;
};
