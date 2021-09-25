#include "FileLoading.h"
#include "../TinyFileDialogs/tinyfiledialogs.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "../MainLoop/MainLoop.h"

#include "../Objects/BaseObject.h"
#include "../Objects/YosemiteTerrain.h"
#include "../Objects/PlayerCharacter.h"
#include "../RenderEngine/RenderEngine.light/DirectionalLight.h"
#include "../RenderEngine/RenderEngine.light/PointLight.h"

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

	//
	// Clear out the whole scene
	//
	for (int i = MainLoop::getInstance().imguiObjects.size() - 1; i >= 0; i--)
	{
		delete MainLoop::getInstance().imguiObjects[i]->baseObject;
	}

	std::cout << "::Opening:: \"" << fname << "\" ..." << std::endl;

	// Load all info of the level
	std::ifstream i(fname);
	nlohmann::json j;
	i >> j;

	/*std::ofstream o("pretty.json");				// TODO: implement this json writer as save function
	o << std::setw(4) << j << std::endl;*/

	//
	// Start working with the retrieved filename
	//
	nlohmann::json& objects = j["objects"];
	for (auto& object : objects)
	{
		loadObjectWithJson(object);
	}
}

void FileLoading::loadObjectWithJson(nlohmann::json& object)
{
	BaseObject* buildingObject = nullptr;

	// @Palette: This is where you add objects to be loaded into the scene
	if (object["type"] == "ground")			buildingObject = new YosemiteTerrain();
	if (object["type"] == "dir_light")		buildingObject = new DirectionalLight(false);
	if (object["type"] == "player")			buildingObject = new PlayerCharacter();
	if (object["type"] == "pt_light")		buildingObject = new PointLight();

	buildingObject->streamTokensForLoading(object);

	assert(buildingObject != nullptr);
}
