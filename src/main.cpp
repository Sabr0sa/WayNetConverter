#include <iostream>
#include <chrono>
#include <filesystem>
#include "Converter.h"

int main(int argc, char *argv[])
{
	std::cout << "Reveares' Waypoint Converter v1.1 (improved by Sabrosa)\n\n";

	if(argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " path/to/file.zen\n";
		return 1;
	}
	std::filesystem::path zen = argv[1];

	auto start = std::chrono::high_resolution_clock::now();

	Converter converter;
	converter.readZen(zen.string());
	converter.writeWp(zen.replace_extension(".wp").string());
	converter.writeFp(zen.replace_extension(".fp").string());

	auto end = std::chrono::high_resolution_clock::now();
	std::cout << "Which took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms.\n";

	return 0;
}
