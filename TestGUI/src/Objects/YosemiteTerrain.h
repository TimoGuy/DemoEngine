#pragma once

#include <glad/glad.h>
#include <PxPhysicsAPI.h>
#include "BaseObject.h"
#include "../RenderEngine/RenderEngine.model/RenderEngine.model.animation/Animator.h"


class YosemiteTerrainImGui : public ImGuiComponent
{
public:
	YosemiteTerrainImGui(BaseObject* bo, Bounds* bounds) : ImGuiComponent(bo, bounds, "Yosemite Terrain") {}

	void propertyPanelImGui();
};

class YosemiteTerrainRender : public RenderComponent
{
public:
	YosemiteTerrainRender(BaseObject* bo, Bounds* bounds, std::string modelResourceName);

	void preRenderUpdate();
	void render();
	void renderShadow(GLuint programId);

	unsigned int pbrShaderProgramId, shadowPassProgramId;

	std::string modelResourceName;
	Model* model;
	std::map<std::string, Material*> materials;

private:
	void refreshResources();
};

class YosemiteTerrain : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	YosemiteTerrain(std::string modelResourceName = "model;cube");
	~YosemiteTerrain();

	void loadPropertiesFromJson(nlohmann::json& object);
	nlohmann::json savePropertiesToJson();

	ImGuiComponent* imguiComponent;
	PhysicsComponent* physicsComponent;
	RenderComponent* renderComponent;

	ImGuiComponent* getImguiComponent() { return imguiComponent; }
	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return physicsComponent; }
	RenderComponent* getRenderComponent() { return renderComponent; }

	void INTERNALrecreatePhysicsComponent(std::string modelResourceName);

	Bounds* bounds;
};

