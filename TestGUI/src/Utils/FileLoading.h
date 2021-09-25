#pragma once

#include <string>
#include <vector>
#include "../Utils/json.hpp"


class BaseObject;

class FileLoading
{
public:
	static FileLoading& getInstance();

	bool isCurrentPathValid() { return !currentWorkingPath.empty(); }
	void loadFileWithPrompt();
	void saveFile(bool withPrompt);

private:
	FileLoading() {}

	void loadObjectWithJson(nlohmann::json& object);

	std::string currentWorkingPath;
};
