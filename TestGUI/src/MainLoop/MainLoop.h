#pragma once

#include <vector>
#include <PxPhysicsAPI.h>

#include "../objects/BaseObject.h"
#include "../render_engine/camera/Camera.h"

class RenderManager;


class MainLoop
{
public:
	static MainLoop& getInstance();

	void initialize();
	void run();
	void cleanup();

	void deleteObject(BaseObject* obj);

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

private:
	std::vector<BaseObject*> objectsToDelete;

	// Fullscreen/windowed cache
	bool isFullscreen;
	int windowXposCache, windowYposCache, windowWidthCache, windowHeightCache;
	void setFullscreen(bool flag, bool force = false);
};
