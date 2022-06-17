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


FileLoading& FileLoading::getInstance()
{
	static FileLoading instance;
	return instance;
}

nlohmann::json FileLoading::loadJsonFile(std::string fname)
{
	nlohmann::json j;
	std::ifstream i(fname);
	if (i.is_open())
	{
		i >> j;
		std::cout << "FILEIO:: File \"" << fname << "\" Successfully loaded (JSON)." << std::endl;
	}
	return j;
}

void FileLoading::saveJsonFile(std::string fname, nlohmann::json& object)
{
	std::ofstream o(fname);
	o << std::setw(4) << object << std::endl;
	std::cout << "FILEIO:: File \"" << fname << "\" Successfully saved (JSON)." << std::endl;
}

char* FileLoading::openFileDialog(const std::string& title, const std::string& startingPath, const char* filters[], const char* filterDescription)
{
	std::string currentPath{ std::filesystem::current_path().string() + std::string("/") + startingPath };
	char* fnameOpened = tinyfd_openFileDialog(
		title.c_str(),
		currentPath.c_str(),
		1,
		filters,
		filterDescription,
		0
	);

	return fnameOpened;
}

char* FileLoading::saveFileDialog(const std::string& title, const std::string& startingPath, const char* filters[], const char* filterDescription)
{
	std::string currentPath{ std::filesystem::current_path().string() + std::string("/") + startingPath };
	char* fname = tinyfd_saveFileDialog(
		title.c_str(),
		currentPath.c_str(),
		1,
		filters,
		filterDescription
	);

	return fname;
}


void FileLoading::loadFileWithPrompt(bool withPrompt)
{
	// Load all info of the level
	std::string fname;
	{
		nlohmann::json j = loadJsonFile("res\\solanine_editor_settings.json");		// @TODO: This kinda bothers me. There's no way to guarantee the user has this file touched. At the very least there should be a "if file doesn't exist, touch it real quick" at the beginning of the program... dango bango yoooo.
		if (!j.empty())
		{
			if (j.contains("startup_level"))
			{
				fname = j["startup_level"];
			}

			if (j.contains("level_editor_camera_pos"))
			{
				MainLoop::getInstance().camera.position = { j["level_editor_camera_pos"][0], j["level_editor_camera_pos"][1], j["level_editor_camera_pos"][2] };
			}

			if (j.contains("level_editor_camera_orientation"))
			{
				MainLoop::getInstance().camera.orientation = { j["level_editor_camera_orientation"][0], j["level_editor_camera_orientation"][1], j["level_editor_camera_orientation"][2] };
			}
		}
	}


	if (withPrompt)
	{
		const char* filters[] = { "*.hsfs" };
		std::string currentPath{ std::filesystem::current_path().string() + std::string("\\res\\level\\") };
		char* fnameOpened = tinyfd_openFileDialog(
			"Open Scene File",
			currentPath.c_str(),
			1,
			filters,
			"Game Scene Files (*.hsfs)",
			0
		);

		if (!fnameOpened)
		{
			std::cout << "ERROR: Adam Sandler" << std::endl;
			return;
		}

		// Set this opened file as the new default for next time you open the program
		// @Copypasta
		nlohmann::json j;
		j["startup_level"] = std::filesystem::relative(fnameOpened).string();
		std::ofstream o("res\\solanine_editor_settings.json");
		o << std::setw(4) << j << std::endl;
		std::cout << "::NOTE:: Set new startup file as \"" << fnameOpened << "\" ..." << std::endl;

		fname = std::string(fnameOpened);
	}

	// Check if file actually exists
	if (fname.empty() || !std::filesystem::exists(fname))
		fname = "res\\level\\level1.hsfs";					// Default filename

	//
	// Clear out the whole scene
	//
	for (int i = (int)MainLoop::getInstance().objects.size() - 1; i >= 0; i--)
	{
		delete MainLoop::getInstance().objects[i];
	}

#ifdef _DEVELOP
	MainLoop::getInstance().renderManager->deselectAllSelectedObject();
#endif

	std::cout << "::Opening:: \"" << fname << "\" ..." << std::endl;
	currentWorkingPath = fname;

	// Load all info of the level
	nlohmann::json j = loadJsonFile(currentWorkingPath);

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
		std::string currentPath{ std::filesystem::current_path().string() + std::string("\\res\\level\\") };
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

		// Set this opened file as the new default for next time you open the program
		nlohmann::json j;
		j["startup_level"] = std::filesystem::relative(fname).u8string();		// This is apparently the whole path, so oh well. The fallback level1.hsfs should be good though
		std::ofstream o("res\\solanine_editor_settings.json");
		o << std::setw(4) << j << std::endl;
		std::cout << "::NOTE:: Set new startup file as \"" << fname << "\" ..." << std::endl;

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

#ifdef _DEVELOP
void FileLoading::saveCameraPosition()
{
	nlohmann::json j;
	{
		j = loadJsonFile("res\\solanine_editor_settings.json");

		// Append the camera information to this
		Camera& cam = MainLoop::getInstance().camera;
		j["level_editor_camera_pos"] = { cam.position.x, cam.position.y, cam.position.z };
		j["level_editor_camera_orientation"] = { cam.orientation.x, cam.orientation.y, cam.orientation.z };
	}

	saveJsonFile("res\\solanine_editor_settings.json", j);
}
#endif
