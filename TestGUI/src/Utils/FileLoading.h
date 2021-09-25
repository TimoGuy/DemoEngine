#pragma once

#include <string>
#include <vector>
#include "../Utils/json.hpp"


class BaseObject;

class FileLoading
{
public:
	static FileLoading& getInstance();

	void loadFileWithPrompt();

private:
	FileLoading() {}

	void loadObjectWithJson(nlohmann::json& object);
};
