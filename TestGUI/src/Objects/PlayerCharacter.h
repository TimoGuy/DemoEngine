#pragma once

#include "BaseObject.h"
#include "../RenderEngine/RenderEngine.model/RenderEngine.model.animation/Animator.h"
#include <glad/glad.h>


class PlayerCharacter : public ImGuiObject, public PhysicsObject, public RenderObject
{
public:
	PlayerCharacter();
	~PlayerCharacter();

	void physicsUpdate(float deltaTime);
	void render(bool shadowPass, unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture, unsigned int shadowMapTexture);
	void propertyPanelImGui();
	void renderImGui() {}		// TODO: implement the imguizmo interfaces so that we can have gizmos

private:
	unsigned int pbrShaderProgramId, shadowPassSkinnedProgramId;

	Model model;
	Animator animator;
	GLuint pbrAlbedoTexture, pbrNormalTexture, pbrMetalnessTexture, pbrRoughnessTexture;
};

