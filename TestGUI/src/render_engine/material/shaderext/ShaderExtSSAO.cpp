#include "ShaderExtSSAO.h"

#include "../Shader.h"
#include "../../../mainloop/MainLoop.h"
#include "../../render_manager/RenderManager.h"


ShaderExtSSAO::ShaderExtSSAO(Shader* shader) : ShaderExt(shader)
{
}

void ShaderExtSSAO::setupExtension()		// @REFACTOR: have rendermanager update the invFullResolution vec2 value! And the ssao texture
{
	shader->setSampler("ssaoTexture", MainLoop::getInstance().renderManager->getSSAOTexture());
	shader->setVec2("invFullResolution", { 1.0f / MainLoop::getInstance().camera.width, 1.0f / MainLoop::getInstance().camera.height });
}
