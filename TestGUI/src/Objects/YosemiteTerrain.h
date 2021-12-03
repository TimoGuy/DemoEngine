#pragma once

#include <PxPhysicsAPI.h>
#include "BaseObject.h"
#include "../RenderEngine/RenderEngine.model/RenderEngine.model.animation/Animator.h"


typedef unsigned int GLuint;

class YosemiteTerrainRender : public RenderComponent
{
public:
	YosemiteTerrainRender(BaseObject* bo, RenderAABB* bounds, std::string modelResourceName);

	void preRenderUpdate();
	void render();
	void renderShadow(GLuint programId);

	unsigned int pbrShaderProgramId, shadowPassProgramId;

	std::string modelResourceName;
	Model* model;
	std::map<std::string, Material*> materials;

	// TODO: This needs to stop (having a bad loading function) bc this function should be private, but it's not. Kuso.
	void refreshResources();
private:
};

class YosemiteTerrain : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	YosemiteTerrain(std::string modelResourceName = "model;cube");
	~YosemiteTerrain();

	void loadPropertiesFromJson(nlohmann::json& object);
	nlohmann::json savePropertiesToJson();

	void physicsUpdate();

	PhysicsComponent* physicsComponent;
	RenderComponent* renderComponent;

	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return physicsComponent; }
	RenderComponent* getRenderComponent() { return renderComponent; }

	void INTERNALrecreatePhysicsComponent(std::string modelResourceName);

#ifdef _DEBUG
	void propertyPanelImGui();
#endif

	RenderAABB* bounds;

	// TODO: this should be private, with all the components just referring back to the single main one
	physx::PxVec3 velocity, angularVelocity;
};

