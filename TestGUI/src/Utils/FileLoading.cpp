#include "FileLoading.h"
#include "../TinyFileDialogs/tinyfiledialogs.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "../Objects/BaseObject.h"
#include "../Objects/YosemiteTerrain.h"
#include "../Objects/PlayerCharacter.h"
#include "../RenderEngine/RenderEngine.light/DirectionalLight.h"

#include "../Utils/Utils.h"


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

	line = trimString(line);

	if (!line.length())			// This should bump out empty lines and just comments as lines
		return;
	
	//
	// Parse out all the information
	//
	std::vector<std::string> tokens;

	std::istringstream iss(line);
	std::string token;
	while (std::getline(iss, token, '\t'))   // Tab delimited
		tokens.push_back(trimString(token));

	//
	// THE ACTUAL LOADING HAPPENS HERE
	//
	if (buildingObject == nullptr)
	{
		// Check if starting a new object
		if (tokens[0] == "new")
		{
			createAndProcessFirstTokenLineOfObject(tokens);
			return;
		}
	}
	else
	{
		// Check if end building object
		if (tokens[0] == "end")
		{
			buildingObject = nullptr;
			return;
		}

		if (buildingObject->streamTokensForLoading(tokens))			// Returning true means that the lexical analyzer found that the line made sense.
			return;
	}

	//
	// Things didn't work out.... say there's an error on line ###
	//
	std::cout << "ERROR (line " << lineNumber << "): could not figger out what the heck this is:" << std::endl << "\t" << line << std::endl;
}

void FileLoading::createAndProcessFirstTokenLineOfObject(std::vector<std::string>& tokens)			// @AddNewObjectsHere
{
	if (trimString(tokens[1]) == "ground")
	{
		buildingObject = new YosemiteTerrain();
		buildingObject->streamTokensForLoading(tokens);
		return;
	}
	if (trimString(tokens[1]) == "player")
	{
		buildingObject = new PlayerCharacter();
		buildingObject->streamTokensForLoading(tokens);
		return;
	}
	if (trimString(tokens[1]) == "dir_light")		// TODO: figure out the loading scheme for this here
	{
		buildingObject = new DirectionalLight(false);
		buildingObject->streamTokensForLoading(tokens);
		return;
	}
}
