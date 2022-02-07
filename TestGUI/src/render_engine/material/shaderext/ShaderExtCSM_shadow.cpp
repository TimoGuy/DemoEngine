#include "ShaderExtCSM_shadow.h"

#include <glad/glad.h>
#include "../../../mainloop/MainLoop.h"
#include "../../render_manager/RenderManager.h"


ShaderExtCSM_shadow::ShaderExtCSM_shadow(unsigned int programId)
{
	depthBufferUniformLoc = glGetUniformLocation(programId, "depthTexture");
}

void ShaderExtCSM_shadow::setupExtension(unsigned int& tex, nlohmann::json* params)
{
	glBindTextureUnit(tex, MainLoop::getInstance().renderManager->getDepthMap());
	glUniform1i(depthBufferUniformLoc, (int)tex);
	tex++;
}
