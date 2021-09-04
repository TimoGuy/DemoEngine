#pragma once

#include <glm/glm.hpp>


enum LightType { DIRECTIONAL, POINT, SPOT };
class Light
{
public:
	glm::vec3 position;
	glm::vec3 orientation;
	glm::vec3 up;

	LightType lightType;
	bool castsShadows;								// The rendermanager will create the shadowmaps from their end, not the individual lights

	virtual glm::mat4 getLightProjection() = 0;
	virtual glm::mat4 getLightView() = 0;
};
