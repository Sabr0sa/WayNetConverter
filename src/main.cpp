#include <iostream>
#include <chrono>
#include <filesystem>
#include "Converter.h"

int main(int argc, char *argv[])
{
	std::cout << "Reveares' Waypoint Converter" << std::endl << std::endl;

	if(argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " path/to/file.zen\n";
		return 1;
	}
	std::filesystem::path zen = argv[1];

	auto start = std::chrono::high_resolution_clock::now();

	Converter converter;
	converter.read(zen.string());
	converter.write(zen.replace_extension(".wp").string());

	auto end = std::chrono::high_resolution_clock::now();
	std::cout << "Which took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms.\n";

	return 0;
}
