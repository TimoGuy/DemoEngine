#pragma once

#include <string>
#include <map>



namespace Resources
{
	std::map<std::string, void*>& getResourceMap();					// Hopefully nobody abuses this :( @Cleanup: make a const return function to get the resourcemap

	void* getResource(const std::string& resourceName, const void* compareResource = nullptr, bool* resourceDiffersAnswer = nullptr);
	void reloadResource(const std::string& resourceName);
	void unloadResource(std::string resourceName);
}