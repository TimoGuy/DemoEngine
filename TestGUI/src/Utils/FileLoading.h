#pragma once

#include <string>


class FileLoading
{
public:
	static FileLoading& getInstance();

	void loadFileWithPrompt();

private:
	FileLoading() {}

	void processLoadSceneLine(std::string& line, size_t lineNumber);
};
