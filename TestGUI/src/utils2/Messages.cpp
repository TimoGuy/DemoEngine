#include "Messages.h"

#include <iostream>


Messages& Messages::getInstance()
{
	static Messages instance;
	return instance;
}

void Messages::postMessage(const std::string& messageName)
{
	messages.push_back(messageName);
}

bool Messages::checkForMessage(const std::string& messageName)
{
	if (messages.size() == 0)
		return false;

	for (size_t i = 0; i < messages.size(); i++)
	{
		if (messages[i] == messageName)
		{
			messages.erase(messages.begin() + i);
			return true;
		}
	}

	return false;
}
