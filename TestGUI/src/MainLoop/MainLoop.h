#pragma once

#include <vector>
#include <PxPhysicsAPI.h>

#include "../Objects/BaseObject.h"
#include "../RenderEngine/RenderEngine.camera/Camera.h"

class RenderManager;


class MainLoop
{
public:
	static MainLoop& getInstance();

	void initialize();
	void run();
	void cleanup();

	GLFWwindow* window;
	Camera camera;
	std::vector<BaseObject*> objects;
	std::vector<LightComponent*> lightObjects;
	std::vector<PhysicsComponent*> physicsObjects;
	std::vector<RenderComponent*> renderObjects;

	RenderManager* renderManager;

	physx::PxScene* physicsScene;
	physx::PxPhysics* physicsPhysics;
	physx::PxCooking* physicsCooking;
	physx::PxControllerManager* physicsControllerManager;
	physx::PxMaterial* defaultPhysicsMaterial;

	float deltaTime;			// To only be used on the rendering thread
	float physicsDeltaTime;		// To only be used on the physics thread
	float physicsCalcTimeAnchor;

#ifdef _DEBUG
	bool playMode = false;
#endif

	//
	// Debug mode flags
	//
	//bool simulatePhysics = false;		@Simplify: this was making things too complicated
};
