#pragma once

#include <string>
#include <vector>


class BaseObject;

class FileLoading
{
public:
	static FileLoading& getInstance();

	void loadFileWithPrompt();

private:
	FileLoading() {}

	BaseObject* buildingObject = nullptr;			// This object will get built up and then set to nullptr again, since its reference will be contained in the mainloop's list of objects (TODO)
	void processLoadSceneLine(std::string& line, size_t lineNumber);
	void createAndProcessFirstTokenLineOfObject(std::vector<std::string>& tokens);
};
