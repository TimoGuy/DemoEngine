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
	std::vector<ImGuiComponent*> imguiObjects;		// NOTE: This is only for debug... imgui renders from here.
	std::vector<LightComponent*> lightObjects;
	std::vector<PhysicsComponent*> physicsObjects;
	std::vector<RenderComponent*> renderObjects;

	RenderManager* renderManager;

	physx::PxScene* physicsScene;
	physx::PxPhysics* physicsPhysics;
	physx::PxControllerManager* physicsControllerManager;
	physx::PxMaterial* defaultPhysicsMaterial;

	float deltaTime;		// NOTE: to only be used on the rendering thread
	bool playMode = false;

	//
	// Debug mode flags
	//
	bool simulatePhysics = false;
};
