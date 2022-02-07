#include "ShaderExtPBR_daynight_cycle.h"

#include <glad/glad.h>
#include "../../../mainloop/MainLoop.h"
#include "../../render_manager/RenderManager.h"


ShaderExtPBR_daynight_cycle::ShaderExtPBR_daynight_cycle(unsigned int programId)
{
	depthBufferUniformLoc = glGetUniformLocation(programId, "depthTexture");
}

void ShaderExtPBR_daynight_cycle::setupExtension(unsigned int& tex, nlohmann::json* params)
{
	glBindTextureUnit(tex, MainLoop::getInstance().renderManager->getDepthMap());
	glUniform1i(depthBufferUniformLoc, (int)tex);
	tex++;
}
