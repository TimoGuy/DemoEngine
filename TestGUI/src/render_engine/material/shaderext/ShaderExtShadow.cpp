#include "ShaderExtShadow.h"

#include "../Shader.h"
#include "../../../mainloop/MainLoop.h"
#include "../../render_manager/RenderManager.h"

unsigned int ShaderExtShadow::spotLightShadows[MAX_SHADOWS];
unsigned int ShaderExtShadow::pointLightShadows[MAX_SHADOWS];
float ShaderExtShadow::pointLightShadowFarPlanes[MAX_SHADOWS];


ShaderExtShadow::ShaderExtShadow(Shader* shader) : ShaderExt(shader)
{
}

void ShaderExtShadow::setupExtension()
{
	for (int i = 0; i < MAX_SHADOWS; i++)
	{
		if (spotLightShadows[i] != 0)
		{
			shader->setSampler("spotLightShadowMaps[" + std::to_string(i) + "]", spotLightShadows[i]);
			continue;
		}

		if (pointLightShadows[i] != 0)
		{
			shader->setSampler("pointLightShadowMaps[" + std::to_string(i) + "]", pointLightShadows[i]);
			shader->setFloat("pointLightShadowFarPlanes[" + std::to_string(i) + "]", pointLightShadowFarPlanes[i]);
			continue;
		}
	}
}
