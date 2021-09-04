#pragma once

#include <PxPhysicsAPI.h>
#include <glm/glm.hpp>

namespace GameLoop
{
	void initialize();
	void run();
	void destroy();
}


namespace GameLoopUtils
{
	glm::mat4 physxMat44ToGlmMat4(physx::PxMat44 mat4);
}
