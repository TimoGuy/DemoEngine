#pragma once

#include "ShaderExt.h"
#include <vector>
#include <glm/glm.hpp>


class ShaderExtCSM_shadow : public ShaderExt
{
public:
	ShaderExtCSM_shadow(Shader* shader);
	void setupExtension();

	static unsigned int csmShadowMap;
	static std::vector<float> cascadePlaneDistances;
	static std::vector<float> cascadeShadowMapTexelSizes;
	static int cascadeCount;
	static float nearPlane;
	static float farPlane;
};
