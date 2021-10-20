#pragma once

#include <map>
#include <vector>
#include <PxPhysicsAPI.h>

#include "../Objects/BaseObject.h"
#include "../RenderEngine/RenderEngine.camera/Camera.h"
#include "../RenderEngine/RenderEngine.model/Mesh.h"

class RenderManager;


class MainLoop
{
public:
	static MainLoop& getInstance();

	void initialize();
	void run();
	void cleanup();

	void destroyObject(BaseObject* object);
	std::vector<BaseObject*> destroyObjectList;

	GLFWwindow* window;
	Camera camera;
	std::vector<ImGuiComponent*> imguiObjects;		// NOTE: This is only for debug... imgui renders from here.
	std::vector<LightComponent*> lightObjects;
	std::vector<PhysicsComponent*> physicsObjects;
	std::vector<RenderComponent*> renderObjects;
	std::map<GLuint, std::vector<Mesh*>> sortedRenderQueue;

	RenderManager* renderManager;

	physx::PxScene* physicsScene;
	physx::PxPhysics* physicsPhysics;
	physx::PxControllerManager* physicsControllerManager;
	physx::PxMaterial* defaultPhysicsMaterial;

	float deltaTime;			// To only be used on the rendering thread
	float physicsDeltaTime;		// To only be used on the physics thread
	bool playMode = false;

	//
	// Debug mode flags
	//
	//bool simulatePhysics = false;		@Simplify: this was making things too complicated
};
