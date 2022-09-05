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

	GLFWwindow* window = nullptr;
	Camera camera;
	std::vector<BaseObject*> objects;
	std::vector<LightComponent*> lightObjects;
	std::vector<PhysicsComponent*> physicsObjects;
	std::vector<RenderComponent*> renderObjects;

	RenderManager* renderManager = nullptr;

	// Physics System
	physx::PxScene* physicsScene = nullptr;
	physx::PxPhysics* physicsPhysics = nullptr;
	physx::PxCooking* physicsCooking = nullptr;
	physx::PxControllerManager* physicsControllerManager = nullptr;
	physx::PxMaterial* defaultPhysicsMaterial = nullptr;

	float deltaTime = 0.0f;				// To only be used on the rendering thread
	float physicsDeltaTime = 0.0f;			// To only be used on the physics thread
	float physicsCalcTimeAnchor = 0.0f;

#ifdef _DEVELOP
	bool playMode = false;
	float timeScale = 1.0f;

	bool timelineViewerMode = false;
#endif

	//
	// Debug mode flags
	//
	//bool simulatePhysics = false;		@Simplify: this was making things too complicated

private:
	std::vector<BaseObject*> objectsToDelete;

	// Fullscreen/windowed cache
	bool isFullscreen = false;
	int windowXposCache, windowYposCache, windowWidthCache, windowHeightCache;
	void setFullscreen(bool flag, bool force = false);
};
