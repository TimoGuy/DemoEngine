#pragma once

#include <glm/glm.hpp>


enum LightType { DIRECTIONAL, POINT, SPOT };
struct Light
{
	LightType lightType;

	glm::vec3 facingDirection;
	glm::vec3 color;
	float colorIntensity;
};
