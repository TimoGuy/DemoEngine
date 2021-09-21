#include "FileLoading.h"
#include "../TinyFileDialogs/tinyfiledialogs.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>


FileLoading& FileLoading::getInstance()
{
	static FileLoading instance;
	return instance;
}

void FileLoading::loadFileWithPrompt()
{
	const char* filters[] = {  "*.hsfs" };
	std::string currentPath{ std::filesystem::current_path().u8string() + "\\res\\" };
	char* fname = tinyfd_openFileDialog(
		"Open Scene File",
		currentPath.c_str(),
		1,
		filters,
		"Game Scene Files (*.hsfs)",
		0
	);

	if (!fname)
	{
		std::cout << "ERROR: Adam Sandler" << std::endl;
		return;
	}

	std::cout << "::Opening:: \"" << fname << "\" ..." << std::endl;

	//
	// Start working with the retrieved filename
	//
	std::ifstream sceneFile(fname);
	if (sceneFile.is_open())
	{
		std::string line;
		size_t lineNum = 1;
		while (std::getline(sceneFile, line))
		{
			processLoadSceneLine(line, lineNum);
			lineNum++;
		}
		sceneFile.close();
	}
	else
	{
		std::cout << "ERROR: unable to open file" << std::endl;
	}
}

void FileLoading::processLoadSceneLine(std::string& line, size_t lineNumber)
{
	std::cout << line << std::endl;

	// Remove comments from the line
	size_t loc = line.find('#');
	if (loc != std::string::npos)
	{
		line = line.substr(0, loc);
	}

	// Trim left and right
	line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
	line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), line.end());

	if (!line.length())
		return;
	
	//
	// Parse out all the information
	//
	std::vector<std::string> tokens;

	std::istringstream iss(line);
	std::string token;
	while (std::getline(iss, token, '\t'))   // Tab delimited
		tokens.push_back(token);

	//
	// Things didn't work out.... say there's an error on line#
	//
	std::cout << "ERROR (line " << lineNumber << "): could not figger out what the heck this is:" << std::endl << "\t" << line << std::endl;
}
