#pragma once

#include <vector>
#include <string>


class Messages
{
public:
	static Messages& getInstance();

	void postMessage(const std::string& messageName);
	bool checkForMessage(const std::string& messageName);

private:
	std::vector<std::string> messages;
};
