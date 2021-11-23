#include "Converter.h"
#include <limits>
#include <iterator>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

void Converter::readZen(std::string_view fileName)
{
	std::ifstream file(fileName.data(), std::ios::binary);

	std::cout << "Read file: " << fileName << '\n';
	if(!file.is_open())
	{
		std::cerr << "Couldn't open file!\n";
		return;
	}
	std::cout << "Reading ...";

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

	bool foundWaynet = false;
	while(std::getline(file, line))
	{
		if(!foundWaynet)
		{
			if(line.find("[% zCVobSpot:") == std::string::npos)
			{
				foundWaynet = (line.find("[WayNet") != std::string::npos);
			}
			else
			{
				Freepoint& fp = freepoints.emplace_back();

				// skip pack, presetName and bbox3DWS
				file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

				// TODO: extract rotation and convert it to GMP angle
				file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

				// skip to the position
				file.ignore(std::numeric_limits<std::streamsize>::max(), ':');
				file >> fp.pos.x >> fp.pos.y >> fp.pos.z;
				// skip to the name
				file.ignore(std::numeric_limits<std::streamsize>::max(), ':');
				file >> fp.name;
			}
		}
		else if(line.find("[way") != std::string::npos)
		{
			// Get object id.
			auto vector = split(line);
			if(vector.size() != 4)
			{
				std::cerr << "Too many split result (" << vector.size() << ") instead of 3 from line:\n'" << line << "'\n";
				return;
			}
			vector[3].erase(vector[3].end() - 1);  // Remove ']'.
			int objectId = std::stoi(vector[3]);

			std::shared_ptr<Waypoint> waypoint;

			// Create a new Waypoint if this line is not a reference.
			if(line.find("\247") == std::string::npos) // §
			{
				waypoint = std::make_shared<Waypoint>();
				waypoints.emplace(objectId, waypoint);

				// skip to the name
				file.ignore(std::numeric_limits<std::streamsize>::max(), ':');
				file >> waypoint->name;
				// jump to the next line
				file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				// skip waterDepth and underWater
				file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				// skip to the position
				file.ignore(std::numeric_limits<std::streamsize>::max(), ':');
				file >> waypoint->pos.x >> waypoint->pos.y >> waypoint->pos.z;
				// skip to the direction
				file.ignore(std::numeric_limits<std::streamsize>::max(), ':');
				[[maybe_unused]] float dY;
				file >> waypoint->dX >> dY >> waypoint->dZ;
			}

			// Don't create a Way for Waypoints.
			if(line.find("[waypoint") == std::string::npos)
			{
				Way way{ objectId, waypoint };
				if(line.find("[wayl") != std::string::npos)
					wayl.push_back(way);
				else if(line.find("[wayr") != std::string::npos)
					wayr.push_back(way);

				// After a left Way there must be a right Way. The difference should never be higher than 1.
				if(wayr.size() > wayl.size() && wayl.size() - 1 > wayr.size())
				{
					std::cerr << "WayNet didn't started with left Way or two right/left Way successively.\n";
				}
			}
		}
	}
	file.close();

	// Fill out empty waypoint pointers.
	for(auto& way : wayl)
	{
		if(!way.waypoint)
		{
			auto result = waypoints.find(way.objectId);
			if(result != waypoints.end())
			{
				way.waypoint = result->second;
			}
			else
			{
				std::cerr << "For id: " << way.objectId << " doesn't exist a Waypoint!\n";
			}
		}
	}
	for(auto& way : wayr)
	{
		if(!way.waypoint)
		{
			auto result = waypoints.find(way.objectId);
			if(result != waypoints.end())
			{
				way.waypoint = result->second;
			}
			else
			{
				std::cerr << "For id: " << way.objectId << " doesn't exist a Waypoint!\n";
			}
		}
	}

	// Insert Waypoint connections.
	for(size_t i = 0; i < wayl.size(); i++)
	{
		auto left = waypoints.find(wayl[i].objectId);
		auto right = waypoints.find(wayr[i].objectId);
		if(left != waypoints.end() && right != waypoints.end())
		{
			left->second->connections.push_back(right->second->name);
			right->second->connections.push_back(left->second->name);
		}
		else
		{
			std::cerr << "Couldn't get Waypoint with object id: " << wayl[i].objectId << " or " << wayr[i].objectId << '\n';
		}
	}

	std::cout << " Done.\nFreepoints: " << freepoints.size() << "; Waypoints: " << waypoints.size() << "; Ways: " << wayl.size() << '\n';
}

std::vector<std::string> Converter::split(const std::string& line) const
{
	std::istringstream iss{line.c_str()};
	return {std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
}

void Converter::writeWp(std::string_view fileName) const
{
	if(waypoints.size() == 0)
	{
		std::cerr << "[WP] Nothing to write.\n";
		return;
	}
	std::ofstream file(fileName.data(), std::ios::binary); // don't convert \n to \r\n on Windows
	std::cout << "\nWrite file: " << fileName << '\n';
	if(!file.is_open())
	{
		std::cerr << "Couldn't open file!\n";
		return;
	}
	std::cout << "Writing ...";

	for(const auto& wp : waypoints)
	{
		auto waypoint = wp.second;
		file << waypoint->name << ";" << waypoint->pos.x << ";" << waypoint->pos.y << ";" << waypoint->pos.z << ";" << waypoint->dX << ";" << waypoint->dZ;
		for(const auto& connection : waypoint->connections)
			file << ";" << connection;
		file << '\n';
	}

	std::cout << " Done.\n";
}

void Converter::writeFp(std::string_view fileName) const
{
	if(freepoints.size() == 0)
	{
		std::cerr << "[FP] Nothing to write.\n";
		return;
	}
	std::ofstream file(fileName.data(), std::ios::binary); // don't convert \n to \r\n on Windows
	std::cout << "\nWrite file: " << fileName << '\n';
	if(!file.is_open())
	{
		std::cerr << "Couldn't open file!\n";
		return;
	}
	std::cout << "Writing ...";

	for(const auto& fp : freepoints)
		file << fp.name << ";" << fp.pos.x << ";" << fp.pos.y << ";" << fp.pos.z << ";" << fp.dX << ";" << fp.dZ << '\n';

	std::cout << " Done.\n";
}
