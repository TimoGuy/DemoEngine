#pragma once

#include <glad/glad.h>
#include <PxPhysicsAPI.h>
#include "BaseObject.h"
#include "../RenderEngine/RenderEngine.model/RenderEngine.model.animation/Animator.h"


class YosemiteTerrainImGui : public ImGuiComponent
{
public:
	YosemiteTerrainImGui(BaseObject* bo) : ImGuiComponent(bo, "Yosemite Terrain") {}

	void propertyPanelImGui();
	void renderImGui();
	void cloneMe();
};

class YosemiteTerrainRender : public RenderComponent
{
public:
	YosemiteTerrainRender(BaseObject* bo);

	void render(unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture);
	void renderShadow(GLuint programId);

	unsigned int pbrShaderProgramId, shadowPassProgramId;

	Model model;
	Animator animator;
	GLuint pbrAlbedoTexture, pbrNormalTexture, pbrMetalnessTexture, pbrRoughnessTexture;
};

class YosemiteTerrain : public BaseObject
{
public:
	YosemiteTerrain();
	~YosemiteTerrain();

	ImGuiComponent* imguiComponent;
	RenderComponent* renderComponent;
};

