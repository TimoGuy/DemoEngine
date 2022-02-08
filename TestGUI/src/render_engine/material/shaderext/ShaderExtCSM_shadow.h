#pragma once

#include "ShaderExt.h"
#include <glm/glm.hpp>


class ShaderExtCSM_shadow : public ShaderExt
{
public:
	ShaderExtCSM_shadow(Shader* shader);
	void setupExtension();

	static unsigned int csmShadowMap;
	static float cascadePlaneDistances[16];
	static int cascadeCount;
	static glm::mat4 cameraView;
	static float nearPlane;
	static float farPlane;
};
