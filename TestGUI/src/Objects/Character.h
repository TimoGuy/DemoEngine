#pragma once

#include "BaseObject.h"
#include "../RenderEngine/RenderEngine.model/RenderEngine.model.animation/Animator.h"
#include <glad/glad.h>


class Character : PhysicsObject, RenderObject
{
public:
	Character();
	~Character();

	void physicsUpdate(float deltaTime);
	void render(bool shadowPass, Camera& camera, unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture, unsigned int shadowMapTexture);

private:
	unsigned int pbrShaderProgramId, shadowPassSkinnedProgramId;

	Model model;
	Animator animator;
	GLuint pbrAlbedoTexture, pbrNormalTexture, pbrMetalnessTexture, pbrRoughnessTexture;
};

