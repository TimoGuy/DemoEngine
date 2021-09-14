#include "BaseObject.h"

#include "../MainLoop/MainLoop.h"


ImGuiComponent::ImGuiComponent(BaseObject* baseObject, char* name) : baseObject(baseObject), name(name)
{
	MainLoop::getInstance().imguiObjects.push_back(this);
}

ImGuiComponent::~ImGuiComponent()
{
	MainLoop::getInstance().imguiObjects.erase(
		std::remove(
			MainLoop::getInstance().imguiObjects.begin(),
			MainLoop::getInstance().imguiObjects.end(),
			this
		),
		MainLoop::getInstance().imguiObjects.end()
	);
}

CameraComponent::CameraComponent(BaseObject* baseObject) : baseObject(baseObject)
{
	//MainLoop::getInstance().cameraObjects.push_back(this);
}

CameraComponent::~CameraComponent()
{
	/*MainLoop::getInstance().cameraObjects.erase(
		std::remove(
			MainLoop::getInstance().cameraObjects.begin(),
			MainLoop::getInstance().cameraObjects.end(),
			this
		),
		MainLoop::getInstance().cameraObjects.end()
	);*/
}

LightComponent::LightComponent(BaseObject* baseObject) : baseObject(baseObject)
{
	MainLoop::getInstance().lightObjects.push_back(this);
}

LightComponent::~LightComponent()
{
	MainLoop::getInstance().lightObjects.erase(
		std::remove(
			MainLoop::getInstance().lightObjects.begin(),
			MainLoop::getInstance().lightObjects.end(),
			this
		),
		MainLoop::getInstance().lightObjects.end()
	);
}

PhysicsComponent::PhysicsComponent(BaseObject* baseObject) : baseObject(baseObject)
{
	MainLoop::getInstance().physicsObjects.push_back(this);
}

PhysicsComponent::~PhysicsComponent()
{
	MainLoop::getInstance().physicsObjects.erase(
		std::remove(
			MainLoop::getInstance().physicsObjects.begin(),
			MainLoop::getInstance().physicsObjects.end(),
			this
		),
		MainLoop::getInstance().physicsObjects.end()
	);
}

RenderComponent::RenderComponent(BaseObject* baseObject) : baseObject(baseObject)
{
	MainLoop::getInstance().renderObjects.push_back(this);
}

RenderComponent::~RenderComponent()
{
	MainLoop::getInstance().renderObjects.erase(
		std::remove(
			MainLoop::getInstance().renderObjects.begin(),
			MainLoop::getInstance().renderObjects.end(),
			this
		),
		MainLoop::getInstance().renderObjects.end()
	);
}