#include "FileLoading.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "../mainloop/MainLoop.h"
#include "../render_engine/render_manager/RenderManager.h"

#include "../objects/BaseObject.h"
#include "../objects/YosemiteTerrain.h"
#include "../objects/PlayerCharacter.h"
#include "../objects/DirectionalLight.h"
#include "../objects/PointLight.h"
#include "../objects/WaterPuddle.h"
#include "../objects/RiverDropoff.h"
#include "../objects/VoxelGroup.h"
#include "../objects/Spline.h"

#include "tinyfiledialogs.h"
#include "Utils.h"


FileLoading& FileLoading::getInstance()
{
	static FileLoading instance;
	return instance;
}

void FileLoading::loadFileWithPrompt(bool withPrompt)
{
	// Load all info of the level
	char* fname = (char*)"res\\level\\level1.hsfs";
	{
		std::ifstream i("res\\solanine_editor_settings.json");
		if (i.is_open())
		{
			nlohmann::json j;
			i >> j;

			if (j.contains("startup_level"))
			{
				fname = (char*)std::string(j["startup_level"]).c_str();		// @NOTE: apparently this is ng
			}
		}
	}


	if (withPrompt)
	{
		const char* filters[] = { "*.hsfs" };
		std::string currentPath{ std::filesystem::current_path().u8string() + "\\res\\level\\" };
		fname = tinyfd_openFileDialog(
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
	}

	//
	// Clear out the whole scene
	//
	for (int i = (int)MainLoop::getInstance().objects.size() - 1; i >= 0; i--)
	{
		delete MainLoop::getInstance().objects[i];
	}

#ifdef _DEVELOP
	MainLoop::getInstance().renderManager->currentSelectedObjectIndex = -1;
#endif

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
	if (object["type"] == VoxelGroup::TYPE_NAME)		buildingObject = new VoxelGroup();
	if (object["type"] == Spline::TYPE_NAME)			buildingObject = new Spline();

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
		std::string currentPath{ std::filesystem::current_path().u8string() + "\\res\\level\\" };
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
	for (BaseObject* bo : MainLoop::getInstance().objects)
	{
		objects.push_back(bo->savePropertiesToJson());
	}

	nlohmann::json fullObject;
	fullObject["objects"] = objects;

	std::ofstream o(currentWorkingPath);
	o << std::setw(4) << fullObject << std::endl;

	std::cout << "::Saving:: DONE!" << std::endl;
}
