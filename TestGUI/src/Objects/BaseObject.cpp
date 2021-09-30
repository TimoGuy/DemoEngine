#include "BaseObject.h"

#include <sstream>
#include <random>
#include <glm/gtx/matrix_interpolation.hpp>

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
	for (uint32_t i = 0; i < len; i++)
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
	transform = glm::mat4(1.0f);
	guid = generate_hex(32);
}

BaseObject::~BaseObject()
{
	// NOTE: I guess this needs to be created for linking errors that occur when this isn't, bc of how destructors work in c++ dang nabbit
}

void BaseObject::loadPropertiesFromJson(nlohmann::json& object)
{
	// Pick up the guid
	if (object.contains("guid"))
		guid = object["guid"];
	glm::vec3 position = glm::vec3(object["position"][0], object["position"][1], object["position"][2]);
	glm::vec3 eulerAngles = glm::vec3(object["rotation"][0], object["rotation"][1], object["rotation"][2]);
	glm::vec3 scale = glm::vec3(object["scale"][0], object["scale"][1], object["scale"][2]);

	transform = glm::translate(glm::mat4(1.0f), position) * glm::toMat4(glm::quat(glm::radians(eulerAngles))) * glm::scale(glm::mat4(1.0f), scale);
}

nlohmann::json BaseObject::savePropertiesToJson()
{
	nlohmann::json j;
	j["guid"] = guid;

	glm::vec3 position = PhysicsUtils::getPosition(transform);
	glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(PhysicsUtils::getRotation(transform)));
	glm::vec3 scale = PhysicsUtils::getScale(transform);
	j["position"] = { position.x, position.y, position.z };
	j["rotation"] = { eulerAngles.x, eulerAngles.y, eulerAngles.z };
	j["scale"] = { scale.x, scale.y, scale.z };
	return j;
}

glm::mat4& BaseObject::getTransform()
{
	return transform;
}

void BaseObject::setTransform(glm::mat4 newTransform, bool propagate)
{
	transform = newTransform;

	if (!propagate)
		return;

	PhysicsComponent* pc = getPhysicsComponent();
	if (pc != nullptr)
		pc->propagateNewTransform(newTransform);
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

void ImGuiComponent::loadPropertiesFromJson(nlohmann::json& object)
{
	// Pick up the name
	name = object["name"];
}

nlohmann::json ImGuiComponent::savePropertiesToJson()
{
	nlohmann::json j;
	j["name"] = name;
	return j;
}

void ImGuiComponent::renderImGui()
{
	if (bounds == nullptr || MainLoop::getInstance().playMode)
		return;
	
	Bounds cookedBounds = PhysicsUtils::fitAABB(*bounds, baseObject->getTransform());

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

	//
	// Setup selection state color
	//
	ImU32 selectionStateColor = ImColor(0.9607843137f, 0.8666666667f, 0.1529411765f);			// Nothing color
	if (MainLoop::getInstance().renderManager->currentSelectedObjectIndex >= 0 &&
		MainLoop::getInstance().imguiObjects[MainLoop::getInstance().renderManager->currentSelectedObjectIndex] == this)
		selectionStateColor = ImColor(0.921568627f, 0.423529412f, 0.901960784f);			// Selected color
	else if (MainLoop::getInstance().renderManager->currentHoveringObjectIndex >= 0 &&
		MainLoop::getInstance().imguiObjects[MainLoop::getInstance().renderManager->currentHoveringObjectIndex] == this)
		selectionStateColor = ImColor(0.980392157f, 0.631372549f, 0.223529412f);			// Hover color

	physx::PxBoxGeometry boxGeometry(cookedBounds.extents.x, cookedBounds.extents.y, cookedBounds.extents.z);
	PhysicsUtils::imguiRenderBoxCollider(
		glm::translate(glm::mat4(1.0f), cookedBounds.center),
		boxGeometry,
		selectionStateColor
	);			// @Cleanup: this seems inefficient... but I'm just a glm beginnner atm

	//
	// Check if wanting to click
	//
	const bool clickPressedCurrent =
		(glfwGetMouseButton(MainLoop::getInstance().window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
	if (!clickPressedPrevious && clickPressedCurrent)
	{
		// Request a click process
		MainLoop::getInstance().renderManager->requestSelectObject(false, this, col);
	}
	else
	{
		// Request a hover process
		MainLoop::getInstance().renderManager->requestSelectObject(true, this, col);
	}
	clickPressedPrevious = clickPressedCurrent;
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

void LightComponent::loadPropertiesFromJson(nlohmann::json& object)
{
	castsShadows = object["casts_shadows"];
}

nlohmann::json LightComponent::savePropertiesToJson()
{
	nlohmann::json j;
	j["casts_shadows"] = castsShadows;
	return j;
}

void LightComponent::renderPassShadowMap() {}

PhysicsComponent::PhysicsComponent(BaseObject* baseObject, Bounds* bounds) : baseObject(baseObject), bounds(bounds)
{
	// Populate the transformstate struct
	physicsTransformState.updateTransform(baseObject->getTransform());
	physicsTransformState.updateTransform(baseObject->getTransform());

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

glm::mat4 PhysicsComponent::getTransform()
{
	return physicsTransformState.getInterpolatedTransform();
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

void PhysicsTransformState::updateTransform(glm::mat4 newTransform)
{
	previousUpdateTime = currentUpdateTime;
	currentUpdateTime = (float)glfwGetTime();
	previousTransform = currentTransform;
	currentTransform = newTransform;
}

glm::mat4 PhysicsTransformState::getInterpolatedTransform()
{
	float time = (float)glfwGetTime();
	time -= MainLoop::getInstance().physicsDeltaTime;
	float interpolationValue = std::clamp((time - previousUpdateTime) / (currentUpdateTime - previousUpdateTime), 0.0f, 1.0f);
	return glm::interpolate(previousTransform, currentTransform, interpolationValue);
}
