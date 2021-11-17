#pragma once

#include <string>
#include <map>
#include <glad/glad.h>


namespace Resources
{
	std::map<std::string, void*>& getResourceMap();					// Hopefully nobody abuses this :( @Cleanup: make a const return function to get the resourcemap

	void* getResource(const std::string& resourceName, const void* compareResource = nullptr, bool* resourceDiffersAnswer = nullptr);
	void reloadResource(const std::string& resourceName);
	// void releaseResource(std::string resourceName);			// TODO: this will be important to implement l8r
}