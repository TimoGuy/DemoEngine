#pragma once

#include <PxPhysicsAPI.h>
#include "BaseObject.h"
#include "../render_engine/model/animation/Animator.h"


typedef unsigned int GLuint;

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

	PhysicsComponent* physicsComponent;
	RenderComponent* renderComponent;

	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return physicsComponent; }
	RenderComponent* getRenderComponent() { return renderComponent; }

	void INTERNALrecreatePhysicsComponent(std::string modelResourceName);

#ifdef _DEBUG
	void imguiPropertyPanel();
#endif

	// TODO: this should be private, with all the components just referring back to the single main one
	physx::PxVec3 velocity, angularVelocity;

	//
	// OLD YOSEMITETERRAINRENDER
	//
	std::string modelResourceName;
	Model* model;
	std::map<std::string, Material*> materials;

	// TODO: This needs to stop (having a bad loading function) bc this function should be private, but it's not. Kuso.
	void refreshResources();
};

