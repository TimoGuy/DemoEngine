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
	static float cascadeShadowMapTexelSize;
	static int cascadeCount;
	static float nearPlane;
	static float farPlane;
};
