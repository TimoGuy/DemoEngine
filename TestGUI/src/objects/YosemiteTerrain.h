#pragma once

#include <PxPhysicsAPI.h>
#include "BaseObject.h"
#include "../render_engine/model/animation/Animator.h"


typedef unsigned int GLuint;
class Material;

class YosemiteTerrain : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	YosemiteTerrain(std::string modelResourceName = "model;cube");
	~YosemiteTerrain();

	void loadPropertiesFromJson(nlohmann::json& object);
	nlohmann::json savePropertiesToJson();

	void preRenderUpdate();
	void physicsUpdate();

	PhysicsComponent* physicsComponent = nullptr;
	RenderComponent* renderComponent = nullptr;

	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return physicsComponent; }
	RenderComponent* getRenderComponent() { return renderComponent; }

	void INTERNALrecreatePhysicsComponent(std::string modelResourceName);

#ifdef _DEVELOP
	void imguiPropertyPanel();
#endif

	// TODO: this should be private, with all the components just referring back to the single main one
	physx::PxVec3 velocity = physx::PxVec3(0.0f),
		angularVelocity = physx::PxVec3(0.0f);

	//
	// OLD YOSEMITETERRAINRENDER
	//
	std::string modelResourceName;
	Model* model;
	std::map<std::string, Material*> materials;

	// TODO: This needs to stop (having a bad loading function) bc this function should be private, but it's not. Kuso.
	void refreshResources();
};

