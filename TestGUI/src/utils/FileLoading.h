#pragma once

#include <string>
#include <vector>
#include "../utils/json.hpp"


class BaseObject;

class FileLoading
{
public:
	static FileLoading& getInstance();
	static nlohmann::json loadJsonFile(std::string fname);
	static void saveJsonFile(std::string fname, nlohmann::json& object);
	static char* openFileDialog(const std::string& title, const std::string& startingPath, const char* filters[], const char* filterDescription);
	static char* saveFileDialog(const std::string& title, const std::string& startingPath, const char* filters[], const char* filterDescription);

	bool isCurrentPathValid() { return !currentWorkingPath.empty(); }
	void loadFileWithPrompt(bool withPrompt);
	void saveFile(bool withPrompt);
	
#ifdef _DEVELOP
	void saveCameraPosition();
#endif

	void createObjectWithJson(nlohmann::json& object);

private:
	FileLoading() {}

	std::string currentWorkingPath;
};
