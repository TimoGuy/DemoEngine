#pragma once

#include "ShaderExt.h"
#include <glm/glm.hpp>

class ShaderExtPBR_daynight_cycle : public ShaderExt
{
public:
	ShaderExtPBR_daynight_cycle(Shader* shader);
	void setupExtension();

	static unsigned int irradianceMap,
		prefilterMap,
		brdfLUT;
};
