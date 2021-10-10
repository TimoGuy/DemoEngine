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
	YosemiteTerrainRender(BaseObject* bo, Bounds* bounds);

	void preRenderUpdate();
	void render(unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture);
	void renderShadow(GLuint programId);

	unsigned int pbrShaderProgramId, shadowPassProgramId;

	Model model;
	Animator animator;
	GLuint pbrAlbedoTexture, pbrNormalTexture, pbrMetalnessTexture, pbrRoughnessTexture;

private:
	void refreshResources();
};

class BoxCollider : public PhysicsComponent
{
public:
	BoxCollider(BaseObject* bo, Bounds* bounds);

	void physicsUpdate();
	void propagateNewTransform(const glm::mat4& newTransform);
	physx::PxTransform getGlobalPose();

	physx::PxBoxGeometry getBoxGeometry();

private:
	physx::PxRigidStatic* body;
	physx::PxShape* shape;
};

class YosemiteTerrain : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	YosemiteTerrain();
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

	Bounds* bounds;
};

