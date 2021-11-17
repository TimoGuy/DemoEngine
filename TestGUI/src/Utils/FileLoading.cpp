#include "FileLoading.h"
#include "../TinyFileDialogs/tinyfiledialogs.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "../MainLoop/MainLoop.h"
#include "../RenderEngine/RenderEngine.manager/RenderManager.h"

#include "../Objects/BaseObject.h"
#include "../Objects/YosemiteTerrain.h"
#include "../Objects/PlayerCharacter.h"
#include "../Objects/DirectionalLight.h"
#include "../Objects/PointLight.h"
#include "../Objects/WaterPuddle.h"
#include "../Objects/RiverDropoff.h"

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
	for (int i = (int)MainLoop::getInstance().imguiObjects.size() - 1; i >= 0; i--)
	{
		delete MainLoop::getInstance().imguiObjects[i]->baseObject;
	}
	MainLoop::getInstance().renderManager->currentSelectedObjectIndex = -1;
	MainLoop::getInstance().renderManager->currentHoveringObjectIndex = -1;

	std::cout << "::Opening:: \"" << fname << "\" ..." << std::endl;
	currentWorkingPath = fname;

	// Load all info of the level
	std::ifstream i(currentWorkingPath);
	nlohmann::json j;
	i >> j;

	//
	// Start working with the retrieved filename
	//
	nlohmann::json& objects = j["objects"];
	for (auto& object : objects)
	{
		createObjectWithJson(object);
	}

	std::cout << "::Opening:: DONE!" << std::endl;
}

void FileLoading::createObjectWithJson(nlohmann::json& object)
{
	BaseObject* buildingObject = nullptr;

	// @Palette: This is where you add objects to be loaded into the scene
	if (object["type"] == PlayerCharacter::TYPE_NAME)	buildingObject = new PlayerCharacter();
	if (object["type"] == YosemiteTerrain::TYPE_NAME)	buildingObject = new YosemiteTerrain();
	if (object["type"] == DirectionalLight::TYPE_NAME)	buildingObject = new DirectionalLight(false);
	if (object["type"] == PointLight::TYPE_NAME)		buildingObject = new PointLight(false);
	if (object["type"] == WaterPuddle::TYPE_NAME)		buildingObject = new WaterPuddle();
	if (object["type"] == RiverDropoff::TYPE_NAME)		buildingObject = new RiverDropoff();

	//
	// @@@@TODO: Because the properties are getting loaded after the constructor has run, then
	// it ends up becoming stale. If a large model is created for YosemiteTerrain(), then you'd have
	// to look at it to allow the render function to run, and then only then does the resource see that
	// the model name has changed, so then it'd reload the model. This won't work in release mode, when there
	// isn't hot-swapping in the models. HEY! Fix this
	//
	buildingObject->loadPropertiesFromJson(object);

	assert(buildingObject != nullptr);
}

void FileLoading::saveFile(bool withPrompt)
{
	if (withPrompt)
	{
		const char* filters[] = { "*.hsfs" };
		std::string currentPath{ std::filesystem::current_path().u8string() + "\\res\\" };
		char* fname = tinyfd_saveFileDialog(
			"Save Scene File As...",
			currentPath.c_str(),
			1,
			filters,
			"Game Scene Files (*.hsfs)"
		);

		if (!fname)
		{
			std::cout << "ERROR: Adam Sandler Saves the Day" << std::endl;
			return;
		}

		// Apply the fname
		currentWorkingPath = fname;
	}

	//
	// Do the saving now!
	//
	std::cout << "::Saving:: \"" << currentWorkingPath << "\" ..." << std::endl;

	std::vector<nlohmann::json> objects;
	for (ImGuiComponent* component : MainLoop::getInstance().imguiObjects)
	{
		objects.push_back(component->baseObject->savePropertiesToJson());
	}

	nlohmann::json fullObject;
	fullObject["objects"] = objects;

	std::ofstream o(currentWorkingPath);
	o << std::setw(4) << fullObject << std::endl;

	std::cout << "::Saving:: DONE!" << std::endl;
}
