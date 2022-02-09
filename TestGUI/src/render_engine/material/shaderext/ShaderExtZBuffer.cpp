#include "ShaderExtZBuffer.h"

#include <glad/glad.h>
#include "../Shader.h"
#include "../../../mainloop/MainLoop.h"
#include "../../render_manager/RenderManager.h"

unsigned int ShaderExtZBuffer::depthTexture;


ShaderExtZBuffer::ShaderExtZBuffer(Shader* shader) : ShaderExt(shader)
{
}

void ShaderExtZBuffer::setupExtension()
{
	shader->setSampler("depthTexture", depthTexture);
}
