#pragma once

#include <string>
#include <vector>
#include "../utils/json.hpp"


class BaseObject;

class FileLoading
{
public:
	static FileLoading& getInstance();

	bool isCurrentPathValid() { return !currentWorkingPath.empty(); }
	void loadFileWithPrompt();
	void saveFile(bool withPrompt);

	void createObjectWithJson(nlohmann::json& object);

private:
	FileLoading() {}

	std::string currentWorkingPath;
};
