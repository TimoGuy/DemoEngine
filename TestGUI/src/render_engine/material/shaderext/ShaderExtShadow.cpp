#include "ShaderExtShadow.h"

#include <glad/glad.h>
#include "../../../mainloop/MainLoop.h"
#include "../../render_manager/RenderManager.h"

unsigned int ShaderExtShadow::spotLightShadows[MAX_SHADOWS];
unsigned int ShaderExtShadow::pointLightShadows[MAX_SHADOWS];
float ShaderExtShadow::pointLightShadowFarPlanes[MAX_SHADOWS];


ShaderExtShadow::ShaderExtShadow(unsigned int programId)
{
	for (int i = 0; i < MAX_SHADOWS; i++)
	{
		spotLightShadowMapsUL[i] = glGetUniformLocation(programId, ("spotLightShadowMaps[" + std::to_string(i) + "]").c_str());
		pointLightShadowMapsUL[i] = glGetUniformLocation(programId, ("pointLightShadowMaps[" + std::to_string(i) + "]").c_str());
		pointLightShadowFarPlanesUL[i] = glGetUniformLocation(programId, ("pointLightShadowFarPlanes[" + std::to_string(i) + "]").c_str());
	}
}

void ShaderExtShadow::setupExtension(unsigned int& tex, nlohmann::json* params)		// @REFACTOR: get rendermanager to insert the texture bindings yo!
{
	for (int i = 0; i < MAX_SHADOWS; i++)
	{
		if (spotLightShadows[i] != 0)
		{
			glBindTextureUnit(tex, spotLightShadows[i]);
			glUniform1i(spotLightShadowMapsUL[i], (int)tex);
			tex++;

			continue;
		}

		if (pointLightShadows[i] != 0)
		{
			glBindTextureUnit(tex, pointLightShadows[i]);
			glUniform1i(pointLightShadowMapsUL[i], (int)tex);
			tex++;

			glUniform1f(pointLightShadowFarPlanesUL[i], pointLightShadowFarPlanes[i]);

			continue;
		}
	}
}
