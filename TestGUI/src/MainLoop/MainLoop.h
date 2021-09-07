#pragma once

#include <vector>

#include "../Objects/BaseObject.h"
#include "../RenderEngine/RenderEngine.camera/Camera.h"

class RenderManager;


class MainLoop
{
public:
	static MainLoop& getInstance()
	{
		static MainLoop instance;
		return instance;
	}

	void initialize();
	void run();
	void cleanup();

	GLFWwindow* window;
	Camera camera;
	std::vector<ImGuiObject*> imguiObjects;		// NOTE: This is only for debug... imgui renders from here.
	std::vector<LightObject*> lightObjects;
	std::vector<PhysicsObject*> physicsObjects;
	std::vector<RenderObject*> renderObjects;

	RenderManager* renderManager;
};
