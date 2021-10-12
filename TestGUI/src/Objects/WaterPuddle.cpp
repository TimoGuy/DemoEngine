#include "WaterPuddle.h"

#include "../MainLoop/MainLoop.h"
#include "../Utils/PhysicsUtils.h"
#include "../Utils/GameState.h"
#include "Components/PhysicsComponents.h"


WaterPuddle::WaterPuddle()
{
	bounds = new Bounds();
	bounds->center = glm::vec3(0.0f);
	bounds->extents = glm::vec3(4.0f, 4.0f, 4.0f);

	imguiComponent = new WaterPuddleImgui(this, bounds);
	physicsComponent = new SphereCollider(this, 4.0f, RigidActorTypes::STATIC, ShapeTypes::TRIGGER);
	renderComponent = new WaterPuddleRender(this, bounds);
}

WaterPuddle::~WaterPuddle()
{
	delete renderComponent;
	delete physicsComponent;
	delete imguiComponent;

	delete bounds;
}

void WaterPuddle::loadPropertiesFromJson(nlohmann::json& object)
{
	BaseObject::loadPropertiesFromJson(object["baseObject"]);
	imguiComponent->loadPropertiesFromJson(object["imguiComponent"]);

	//
	// I'll take the leftover tokens then
	//
}

nlohmann::json WaterPuddle::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();
	j["imguiComponent"] = imguiComponent->savePropertiesToJson();

	return j;
}

void WaterPuddle::onTrigger(const physx::PxTriggerPair& pair)
{
	if (pair.otherActor != GameState::getInstance().playerActorPointer)
		return;

	if (pair.status & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
	{
		beingTriggeredByPlayer = true;
		GameState::getInstance().requestTriggerHold(physicsComponent->getActor());
	}
	else if (pair.status & physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
	{
		beingTriggeredByPlayer = false;
		GameState::getInstance().requestTriggerRelease(physicsComponent->getActor());
	}
}

void WaterPuddle::collectWaterPuddle()
{
	std::cout << "Collected!!!!" << std::endl;
	MainLoop::getInstance().destroyObject(this);
}

WaterPuddleRender::WaterPuddleRender(BaseObject* bo, Bounds* bounds) : RenderComponent(bo, bounds)
{
}

void WaterPuddleRender::preRenderUpdate()
{
	if (!((WaterPuddle*)baseObject)->isBeingTriggeredByPlayer())
		return;

	static bool prevEBtnPressed = GLFW_RELEASE;
	bool EBtnpressed = glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_E);
	if (prevEBtnPressed == GLFW_RELEASE && EBtnpressed == GLFW_PRESS)
	{
		//
		// Try to collect this object!
		//
		if (baseObject->getPhysicsComponent()->getActor() == GameState::getInstance().getCurrentTriggerHold())
		{
			// Success!
			((WaterPuddle*)baseObject)->collectWaterPuddle();
		}
	}
	prevEBtnPressed = EBtnpressed;
}

void WaterPuddleRender::render(unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture)
{
}

void WaterPuddleRender::renderShadow(GLuint programId)
{
}

void WaterPuddleImgui::propertyPanelImGui()
{
}

void WaterPuddleImgui::renderImGui()
{
	ImGuiComponent::renderImGui();
}
