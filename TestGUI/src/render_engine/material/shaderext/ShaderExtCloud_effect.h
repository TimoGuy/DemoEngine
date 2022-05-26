#pragma once

#include "ShaderExt.h"
#include <vector>
#include <glm/glm.hpp>


class ShaderExtCloud_effect : public ShaderExt
{
public:
	ShaderExtCloud_effect(Shader* shader);
	void setupExtension();

	static unsigned int cloudEffect;
	static unsigned int cloudDepthTexture;
	static unsigned int atmosphericScattering;
	static glm::vec3 mainCameraPosition;
	static float cloudEffectDensity;
};
