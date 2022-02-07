#include "ShaderExtSSAO.h"

#include <glad/glad.h>
#include "../../../mainloop/MainLoop.h"
#include "../../render_manager/RenderManager.h"


ShaderExtSSAO::ShaderExtSSAO(unsigned int programId)
{
	ssaoTextureUL = glGetUniformLocation(programId, "ssaoTexture");
	invFullResUL = glGetUniformLocation(programId, "invFullResolution");
}

void ShaderExtSSAO::setupExtension(unsigned int& tex, nlohmann::json* params)
{
	glBindTextureUnit(tex, MainLoop::getInstance().renderManager->getSSAOTexture());
	glUniform1i(ssaoTextureUL, (int)tex);
	tex++;

	glUniform2f(invFullResUL, 1.0f / MainLoop::getInstance().camera.width, 1.0f / MainLoop::getInstance().camera.height);
}
