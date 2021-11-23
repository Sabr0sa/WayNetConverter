#include "Converter.h"
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <iterator>

void Converter::read(std::string_view fileName)
{
	std::ifstream file(fileName.data(), std::ifstream::binary);

	std::cout << "Read file: " << fileName << std::endl;
	if (!file.is_open())
	{
		std::cerr << "Couldn't open file!" << std::endl;
		return;
	}
	std::cout << "Reading... " << std::endl;

	// Connections.
	std::vector<Way> wayl;
	std::vector<Way> wayr;

	// skip the first 3 lines to get to the file format description
	file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	std::string line;
	file >> line;
	if(line != "ASCII")
	{
		std::cerr << "Can't read binary .zen files (yet)! Please use an ASCII version.\n";
		exit(1);
	}

	// Skip until '[WayNet' tag.
	bool foundWayNet = false;
	while (std::getline(file, line))
	{
		if (!foundWayNet)
		{
			foundWayNet = line.find("[WayNet") != std::string::npos;
		}
		else if (line.find("[way") != std::string::npos)
		{
			// Get object id.
			auto vector = split(line);
			if (vector.size() != 4)
			{
				std::cerr << "Too many split result (" << vector.size() << ") instead of 3 from line:" << std::endl << "'" << line << "'" << std::endl;
				return;
			}
			vector[3].erase(vector[3].end() - 1);  // Remove ']'.
			int objectId = std::stoi(vector[3]);

			std::shared_ptr<Waypoint> waypoint;

			// Create a new Waypoint if this line is not a reference.
			if(line.find("\247") == std::string::npos) // §
			{
				std::string attribute;
				size_t location;
				std::string lineWaypoint;

				// Get name.
				std::getline(file, lineWaypoint);
				attribute = "wpName=string:";
				location = lineWaypoint.find(attribute);
				if (location == std::string::npos)
				{
					std::cerr << "Awaited '" << attribute << "' but instead line is:" << std::endl << "'" << lineWaypoint << "'" << std::endl;
					return;
				}
				std::string name(lineWaypoint.substr(location + attribute.length()));

				// Skip waterDepth and underWater.
				std::getline(file, lineWaypoint);
				std::getline(file, lineWaypoint);

				// Get position
				std::getline(file, lineWaypoint);
				attribute = "position=vec3:";
				location = lineWaypoint.find(attribute);
				if (location == std::string::npos)
				{
					std::cerr << "Awaited '" << attribute << "' but instead line is:" << std::endl << "'" << lineWaypoint << "'" << std::endl;
					return;
				}
				std::vector<std::string> positions = split(lineWaypoint.substr(location + attribute.length()));
				if (positions.size() != 3)
				{
					std::cerr << "Too many split result (" << positions.size() << ") instead of 3 from line:" << std::endl << "'" << lineWaypoint << "'" << std::endl;
					return;
				}
				double x = std::stod(positions[0]);
				double y = std::stod(positions[1]);
				double z = std::stod(positions[2]);

				// Get direction
				std::getline(file, lineWaypoint);
				attribute = "direction=vec3:";
				location = lineWaypoint.find(attribute);
				if (location == std::string::npos)
				{
					std::cerr << "Awaited '" << attribute << "' but instead line is:" << std::endl << "'" << lineWaypoint << "'" << std::endl;
					return;
				}
				std::vector<std::string> directions = split(lineWaypoint.substr(location + attribute.length()));
				if (directions.size() != 3)
				{
					std::cerr << "Too many split result (" << directions.size() << ") instead of 3 from line:" << std::endl << "'" << lineWaypoint << "'" << std::endl;
					return;
				}
				double dX = std::stod(directions[0]);
				double dZ = std::stod(directions[2]);

				waypoint = std::make_shared<Waypoint>(name, x, y, z, dX, dZ);
				waypoints[objectId] = waypoint;
			}

			// Don't create a Way for Waypoints.
			if (line.find("[waypoint") == std::string::npos)
			{
				Way way{ objectId, waypoint };
				if (line.find("[wayl") != std::string::npos)
				{
					wayl.push_back(way);
				}
				else if (line.find("[wayr") != std::string::npos)
				{
					wayr.push_back(way);
				}

				// After a left Way there must be a right Way. The difference should never be higher than 1.
				if (wayr.size() > wayl.size() && wayl.size() - 1 > wayr.size())
				{
					std::cerr << "WayNet didn't started with left Way or two right/left Way successively." << std::endl;
				}
			}
		}
	}
	file.close();

	// Fill out empty waypoint pointers.
	for (auto& way : wayl)
	{
		if (way.waypoint == nullptr)
		{
			auto result = waypoints.find(way.objectId);
			if (result != waypoints.end())
			{
				way.waypoint = result->second;
			}
			else
			{
				std::cerr << "For id: " << way.objectId << " doesn't exists a Waypoint!" << std::endl;
			}
		}
	}
	for (auto& way : wayr)
	{
		if (way.waypoint == nullptr)
		{
			auto result = waypoints.find(way.objectId);
			if (result != waypoints.end())
			{
				way.waypoint = result->second;
			}
			else
			{
				std::cerr << "For id: " << way.objectId << " doesn't exists a Waypoint!" << std::endl;
			}
		}
	}

	// Insert Waypoint connections.
	for (int i = 0; i < wayl.size(); i++)
	{
		auto left = waypoints.find(wayl[i].objectId);
		auto right = waypoints.find(wayr[i].objectId);
		if (left != waypoints.end() && right != waypoints.end())
		{
			left->second->connections.push_back(right->second->name);
			right->second->connections.push_back(left->second->name);
		}
		else
		{
			std::cerr << "Couldn't get Waypoint with object id: " << wayl[i].objectId << " or " << wayr[i].objectId  << std::endl;
		}
	}

	std::cout << "Done. Waypoints: " << waypoints.size() << " and Ways: " << wayl.size() << std::endl;
}

std::vector<std::string> Converter::split(const std::string& line) const
{
	std::istringstream iss(line);
	return {std::istream_iterator<std::string>{ iss }, std::istream_iterator<std::string>{}};
}

void Converter::write(std::string_view fileName) const
{
	if (waypoints.size() == 0)
	{
		std::cerr << "Nothing to write." << std::endl;
		return;
	}

	std::ofstream file(fileName.data());

	std::cout << std::endl << "Write file: " << fileName << std::endl;
	if (!file.is_open())
	{
		std::cerr << "Couldn't open file!" << std::endl;
		return;
	}
	std::cout << "Writing... " << std::endl;

	for (const auto& wp : waypoints)
	{
		auto waypoint = wp.second;
		file << waypoint->name << ";" << waypoint->x << ";" << waypoint->y << ";" << waypoint->z << ";" << waypoint->dX << ";" << waypoint->dZ;
		for (const auto& connection : waypoint->connections)
		{
			file << ";" << connection;
		}
		file << std::endl;
	}

	std::cout << "Done. Waypoints: " << waypoints.size() << std::endl;
}
