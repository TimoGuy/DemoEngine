#pragma once

#include <string>
#include <map>



namespace Resources
{
	void allowAsyncResourcesToFinishLoading();		// This is so that async resources that still need to do a GL call to stuff stuff into the gpu can do it. (To be used on the main thread in a specific one spot)

	std::map<std::string, void*>& getResourceMap();					// Hopefully nobody abuses this :( @Cleanup: make a const return function to get the resourcemap

	void* getResource(const std::string& resourceName, const void* compareResource = nullptr, bool* resourceDiffersAnswer = nullptr);
	void reloadResource(const std::string& resourceName);
	// void releaseResource(std::string resourceName);			// TODO: this will be important to implement l8r
}