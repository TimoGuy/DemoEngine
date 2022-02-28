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
