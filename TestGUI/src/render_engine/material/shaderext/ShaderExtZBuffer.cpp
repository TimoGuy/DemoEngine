#include "ShaderExtZBuffer.h"

#include <glad/glad.h>
#include "../../../mainloop/MainLoop.h"
#include "../../render_manager/RenderManager.h"


ShaderExtZBuffer::ShaderExtZBuffer(unsigned int programId)
{
	depthBufferUniformLoc = glGetUniformLocation(programId, "depthTexture");
}

void ShaderExtZBuffer::setupExtension(unsigned int& tex, nlohmann::json* params)
{
	glBindTextureUnit(tex, MainLoop::getInstance().renderManager->getDepthMap());
	glUniform1i(depthBufferUniformLoc, (int)tex);
	tex++;
}
