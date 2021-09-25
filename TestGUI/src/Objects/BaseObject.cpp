#include "BaseObject.h"

#include <sstream>
#include <random>

#include "../MainLoop/MainLoop.h"
#include "../Utils/PhysicsUtils.h"
#include "../Utils/Utils.h"
#include "../RenderEngine/RenderEngine.manager/RenderManager.h"


//
// TODO: move this into a Create GUID util code
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

BaseObject::BaseObject()		// TODO: Make baseobject add into a vector containing pointers of these in mainloop for scene cleanup!!!!!
{
	guid = generate_hex(32);
}

BaseObject::~BaseObject()
{
	// NOTE: I guess this needs to be created for linking errors that occur when this isn't, bc of how destructors work in c++ dang nabbit
}

void BaseObject::streamTokensForLoading(nlohmann::json& object)
{
	// Pick up the guid
	guid = object["guid"];
	glm::vec3 position = glm::vec3(object["position"][0], object["position"][1], object["position"][2]);
	glm::vec3 eulerAngles = glm::vec3(object["rotation"][0], object["rotation"][1], object["rotation"][2]);
	glm::vec3 scale = glm::vec3(object["scale"][0], object["scale"][1], object["scale"][2]);

	transform = glm::translate(glm::mat4(1.0f), position) * glm::toMat4(glm::quat(glm::radians(eulerAngles))) * glm::scale(glm::mat4(1.0f), scale);
}

ImGuiComponent::ImGuiComponent(BaseObject* baseObject, Bounds* bounds, std::string name) : baseObject(baseObject), bounds(bounds), name(name)
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

void ImGuiComponent::streamTokensForLoading(nlohmann::json& object)
{
	// Pick up the name
	name = object["name"];
}

void ImGuiComponent::renderImGui()
{
	if (bounds != nullptr)
	{
		Bounds cookedBounds = PhysicsUtils::fitAABB(*bounds, baseObject->transform);

		double xpos, ypos;
		glfwGetCursorPos(MainLoop::getInstance().window, &xpos, &ypos);
		xpos /= MainLoop::getInstance().camera.width;
		ypos /= MainLoop::getInstance().camera.height;
		ypos = 1.0 - ypos;
		xpos = xpos * 2.0f - 1.0f;
		ypos = ypos * 2.0f - 1.0f;
		glm::vec3 clipSpacePosition(xpos, ypos, 1.0f);
		glm::vec3 worldSpacePosition = MainLoop::getInstance().camera.clipSpacePositionToWordSpace(clipSpacePosition);

		PhysicsUtils::RaySegmentHit col = PhysicsUtils::raySegmentCollideWithAABB(
			MainLoop::getInstance().camera.position,
			worldSpacePosition,
			cookedBounds
		);

		physx::PxBoxGeometry boxGeometry(cookedBounds.extents.x, cookedBounds.extents.y, cookedBounds.extents.z);
		PhysicsUtils::imguiRenderBoxCollider(
			glm::translate(glm::mat4(1.0f), cookedBounds.center),
			boxGeometry,
			col.hit ?
				ImColor(0.6392156863f, 0.4078431373f, 0.0470588235f) :
				ImColor(0.9607843137f, 0.8666666667, 0.1529411765)
		);			// @Cleanup: this seems inefficient... but I'm just a glm beginnner atm

		//
		// Check if wanting to click
		//
		if (!col.hit) return;

		const bool clickPressedCurrent =
			(glfwGetMouseButton(MainLoop::getInstance().window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
		if (!clickPressedPrevious && clickPressedCurrent)
		{
			MainLoop::getInstance().renderManager->requestSelectObject(this, col);
			//std::cout << "I did it..." << name << std::endl;
		}
		clickPressedPrevious = clickPressedCurrent;
	}
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

LightComponent::LightComponent(BaseObject* baseObject, bool castsShadows) : baseObject(baseObject), castsShadows(castsShadows)
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

void LightComponent::streamTokensForLoading(nlohmann::json& object)
{
	castsShadows = object["casts_shadows"];
}

void LightComponent::renderPassShadowMap() {}

PhysicsComponent::PhysicsComponent(BaseObject* baseObject, Bounds* bounds) : baseObject(baseObject), bounds(bounds)
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

RenderComponent::RenderComponent(BaseObject* baseObject, Bounds* bounds) : baseObject(baseObject), bounds(bounds)
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