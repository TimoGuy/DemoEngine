#pragma once

#include <glm/glm.hpp>


enum LightType { DIRECTIONAL, POINT, SPOT };
struct Light
{
	LightType lightType;

	glm::vec3 position;
	glm::vec3 facingDirection;

	bool castsShadows;								// The rendermanager will create the shadowmaps from their end, not the individual lights
	glm::mat4 lightProjectionMatrix;
	glm::mat4 lightViewMatrix;
};
