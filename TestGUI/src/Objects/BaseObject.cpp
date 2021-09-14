#include "BaseObject.h"

#include <sstream>
#include <random>

#include "../MainLoop/MainLoop.h"


//
// TODO: make this Create GUID util code
//
uint32_t random_char()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 255);
	return dis(gen);
}

std::string generate_hex(const uint32_t len)
{
	std::stringstream ss;
	for (auto i = 0; i < len; i++)
	{
		const auto rc = random_char();
		std::stringstream hexstream;
		hexstream << std::hex << rc;
		auto hex = hexstream.str();
		ss << (hex.length() < 2 ? '0' + hex : hex);
	}
	return ss.str();
}

ImGuiComponent::ImGuiComponent(BaseObject* baseObject, std::string name) : baseObject(baseObject), name(name)
{
	guid = generate_hex(32);
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