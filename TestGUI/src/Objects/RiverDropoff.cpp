#include "RiverDropoff.h"

#include "../MainLoop/MainLoop.h"
#include "../Utils/PhysicsUtils.h"
#include "../Utils/GameState.h"
#include "Components/PhysicsComponents.h"


RiverDropoff::RiverDropoff()
{
	bounds = new Bounds();
	bounds->center = glm::vec3(0.0f);
	bounds->extents = glm::vec3(1.0f, 1.0f, 1.0f);

	imguiComponent = new RiverDropoffImgui(this, bounds);
	physicsComponent = new BoxCollider(this, bounds, RigidActorTypes::STATIC, ShapeTypes::TRIGGER);
	renderComponent = new RiverDropoffRender(this, bounds);
}

RiverDropoff::~RiverDropoff()
{
	delete renderComponent;
	delete physicsComponent;
	delete imguiComponent;

	delete bounds;
}

void RiverDropoff::loadPropertiesFromJson(nlohmann::json& object)
{
	BaseObject::loadPropertiesFromJson(object["baseObject"]);
	imguiComponent->loadPropertiesFromJson(object["imguiComponent"]);

	//
	// I'll take the leftover tokens then
	//
}

nlohmann::json RiverDropoff::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();
	j["imguiComponent"] = imguiComponent->savePropertiesToJson();

	return j;
}

void RiverDropoff::onTrigger(const physx::PxTriggerPair& pair)
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

void RiverDropoff::dropoffWater()
{
	if (!GameState::getInstance().playerIsHoldingWater)
		return;

	std::cout << "Dropped Off!!!!" << std::endl;
	GameState::getInstance().playerIsHoldingWater = false;
}

RiverDropoffRender::RiverDropoffRender(BaseObject* bo, Bounds* bounds) : RenderComponent(bo, bounds)
{
}

void RiverDropoffRender::preRenderUpdate()
{
	if (!((RiverDropoff*)baseObject)->isBeingTriggeredByPlayer())
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
			((RiverDropoff*)baseObject)->dropoffWater();
		}
	}
	prevEBtnPressed = EBtnpressed;
}

void RiverDropoffRender::render(unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture)
{
}

void RiverDropoffRender::renderShadow(GLuint programId)
{
}

void RiverDropoffImgui::propertyPanelImGui()
{
}

void RiverDropoffImgui::renderImGui()
{
	ImGuiComponent::renderImGui();
}
