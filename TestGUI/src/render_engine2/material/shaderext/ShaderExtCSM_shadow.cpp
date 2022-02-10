#include "ShaderExtCSM_shadow.h"

#include "../Shader.h"
#include "../../../mainloop/MainLoop.h"
#include "../../render_manager/RenderManager.h"


unsigned int ShaderExtCSM_shadow::csmShadowMap;
std::vector<float> ShaderExtCSM_shadow::cascadePlaneDistances;
int ShaderExtCSM_shadow::cascadeCount;
float ShaderExtCSM_shadow::nearPlane;
float ShaderExtCSM_shadow::farPlane;


ShaderExtCSM_shadow::ShaderExtCSM_shadow(Shader* shader) : ShaderExt(shader)
{
}

void ShaderExtCSM_shadow::setupExtension()
{
	shader->setSampler("csmShadowMap", csmShadowMap);
	for (size_t i = 0; i < glm::min((size_t)16, cascadePlaneDistances.size()); i++)
		shader->setFloat("cascadePlaneDistances[" + std::to_string(i) + "]", cascadePlaneDistances[i]);
	shader->setInt("cascadeCount", cascadeCount);
	shader->setFloat("nearPlane", nearPlane);
	shader->setFloat("farPlane", farPlane);
}
