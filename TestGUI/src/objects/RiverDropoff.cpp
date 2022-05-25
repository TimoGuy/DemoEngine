#include "RiverDropoff.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include "../mainloop/MainLoop.h"
#include "../utils/PhysicsUtils.h"
#include "../utils/GameState.h"
#include "../utils/InputManager.h"
#include "../utils/Messages.h"
#include "../render_engine/resources/Resources.h"
#include "../render_engine/model/Model.h"
#include "components/PhysicsComponents.h"


RiverDropoff::RiverDropoff()
{
	name = "River Dropoff Area";

	physicsComponent = new BoxCollider(this, glm::vec3(1.0f), RigidActorTypes::STATIC, ShapeTypes::TRIGGER);
	
	refreshResources();
	renderComponent = new RenderComponent(this);
	renderComponent->addModelToRender({ model, false, nullptr });
}

RiverDropoff::~RiverDropoff()
{
	delete renderComponent;
	delete physicsComponent;
}

void RiverDropoff::loadPropertiesFromJson(nlohmann::json& object)
{
	BaseObject::loadPropertiesFromJson(object["baseObject"]);

	//
	// I'll take the leftover tokens then
	//
}

nlohmann::json RiverDropoff::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();

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
	// @TODO: rework the riverdropoff scheme. I think that just having the metric amount of water you're holding should be the "currency" for knowing whether you can drop off water or not.
	//if (!GameState::getInstance().playerIsHoldingWater)
	//{
	//	std::cout << "This is where you drop off water to send off to your sister (make this a textbox)" << std::endl;
	//	return;
	//}
	//
	//std::cout << "Dropped Off!!!!" << std::endl;
	//Messages::getInstance().postMessage("PlayerCollectWater");
	//GameState::getInstance().playerIsHoldingWater = false;
}

void RiverDropoff::preRenderUpdate()
{
	if (!isBeingTriggeredByPlayer() ||
		getPhysicsComponent()->getActor() != GameState::getInstance().getCurrentTriggerHold())
		return;

	if (InputManager::getInstance().on_interactPressed)
	{
		//
		// Drop off the water!
		//
		dropoffWater();
	}
}

void RiverDropoff::refreshResources()
{
	model = (Model*)Resources::getResource("model;custommodel;res/_debug/trigger_repr_cube.glb");
	materials["Material"] = (Material*)Resources::getResource("material;pbrSlimeVest");
	model->setMaterials(materials);
}

#ifdef _DEVELOP
void RiverDropoff::imguiPropertyPanel()
{
}

void RiverDropoff::imguiRender()
{
}
#endif
