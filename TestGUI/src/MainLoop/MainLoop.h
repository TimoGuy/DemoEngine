#pragma once

#include <vector>

#include "../Objects/BaseObject.h"

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

	std::vector<ImGuiObject*> imguiObjects;		// NOTE: This is only for debug... imgui renders from here.
	std::vector<CameraObject*> cameraObjects;
	std::vector<LightObject*> lightObjects;
	std::vector<PhysicsObject*> physicsObjects;
	std::vector<RenderObject*> renderObjects;

	RenderManager* renderManager;
};
