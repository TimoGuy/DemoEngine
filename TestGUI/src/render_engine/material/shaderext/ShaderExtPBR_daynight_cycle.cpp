#include "ShaderExtPBR_daynight_cycle.h"

#include "../Shader.h"
#include "../../../mainloop/MainLoop.h"
#include "../../render_manager/RenderManager.h"

unsigned int ShaderExtPBR_daynight_cycle::irradianceMap,
	ShaderExtPBR_daynight_cycle::prefilterMap,
	ShaderExtPBR_daynight_cycle::brdfLUT;


ShaderExtPBR_daynight_cycle::ShaderExtPBR_daynight_cycle(Shader* shader) : ShaderExt(shader)
{
}


void ShaderExtPBR_daynight_cycle::setupExtension()
{
	shader->setSampler("irradianceMap", irradianceMap);
	shader->setSampler("prefilterMap", prefilterMap);
	shader->setSampler("brdfLUT", brdfLUT);
}
