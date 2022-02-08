#include "ShaderExtZBuffer.h"

#include <glad/glad.h>
#include "../Shader.h"
#include "../../../mainloop/MainLoop.h"
#include "../../render_manager/RenderManager.h"


ShaderExtZBuffer::ShaderExtZBuffer(Shader* shader) : ShaderExt(shader)
{
}

void ShaderExtZBuffer::setupExtension()		// @REFACTOR: have rendermanager drop in that depth map texture!!!
{
	shader->setSampler("depthTexture", MainLoop::getInstance().renderManager->getDepthMap());
}
