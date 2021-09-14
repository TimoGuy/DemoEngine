#pragma once

#include <string>
#include <glad/glad.h>


namespace Resources
{
	GLuint getResource(const std::string& resourceName);
	// void releaseResource(std::string resourceName);			// TODO: this will be important to implement l8r
}