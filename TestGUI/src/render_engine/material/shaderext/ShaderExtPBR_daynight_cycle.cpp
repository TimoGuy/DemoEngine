#include "ShaderExtPBR_daynight_cycle.h"

#include "../Shader.h"
#include "../../../mainloop/MainLoop.h"
#include "../../render_manager/RenderManager.h"

unsigned int ShaderExtPBR_daynight_cycle::irradianceMap,
	//ShaderExtPBR_daynight_cycle::irradianceMap2,
	ShaderExtPBR_daynight_cycle::prefilterMap,
	//ShaderExtPBR_daynight_cycle::prefilterMap2,
	ShaderExtPBR_daynight_cycle::brdfLUT;
//float ShaderExtPBR_daynight_cycle::mapInterpolationAmt;
//glm::mat3 ShaderExtPBR_daynight_cycle::sunSpinAmount;


ShaderExtPBR_daynight_cycle::ShaderExtPBR_daynight_cycle(Shader* shader) : ShaderExt(shader)
{
}


void ShaderExtPBR_daynight_cycle::setupExtension()
{
	shader->setSampler("irradianceMap", irradianceMap);
	//shader->setSampler("irradianceMap2", irradianceMap2);
	shader->setSampler("prefilterMap", prefilterMap);
	//shader->setSampler("prefilterMap2", prefilterMap2);
	shader->setSampler("brdfLUT", brdfLUT);

	//shader->setFloat("mapInterpolationAmt", mapInterpolationAmt);
	//shader->setMat3("sunSpinAmount", sunSpinAmount);
}
