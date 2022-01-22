#include "WaterPuddle.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include "../mainloop/MainLoop.h"
#include "../utils/PhysicsUtils.h"
#include "../utils/GameState.h"
#include "../utils/InputManager.h"
#include "../utils/Messages.h"
#include "components/PhysicsComponents.h"
#include "../render_engine/resources/Resources.h"
#include "../render_engine/render_manager/RenderManager.h"


WaterPuddle::WaterPuddle() : containsWater(true)
{
	name = "Water Puddle";

	physicsComponent = new SphereCollider(this, 4.0f, RigidActorTypes::STATIC, ShapeTypes::TRIGGER);

	refreshResources();
	renderComponent = new RenderComponent(this);
	model->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0, -2.1f, 0));
	renderComponent->addModelToRender({ model, true, &animator });
}

WaterPuddle::~WaterPuddle()
{
	delete renderComponent;
	delete physicsComponent;
}

void WaterPuddle::loadPropertiesFromJson(nlohmann::json& object)
{
	BaseObject::loadPropertiesFromJson(object["baseObject"]);

	if (object.contains("contains_water"))
		containsWater = object["contains_water"];

	refreshResources();

	// Check if I'm already collected
	if (GameState::getInstance().isPuddleCollected(guid))
		containsWater = false;
}

nlohmann::json WaterPuddle::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();

	j["contains_water"] = containsWater;

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
	if (!containsWater && !GameState::getInstance().playerIsHoldingWater)
	{
		std::cout << "This water puddle is already collected. Unfortunate." << std::endl;
		return;
	}
	if (containsWater && GameState::getInstance().playerIsHoldingWater)
	{
		std::cout << "You're already carrying water. Drop if off or drink it to carry more" << std::endl;
		return;
	}

	// Event!!!
	if (containsWater)
	{
		// Collect and log the puddle
		Messages::getInstance().postMessage("PlayerCollectWater");
		GameState::getInstance().playerIsHoldingWater = true;
		GameState::getInstance().addPuddleGUID(guid);
		//GameState::getInstance().requestTriggerRelease(physicsComponent->getActor());

		std::cout << "Collected Water in Puddle!!!!" << std::endl;
		containsWater = false;
	}
	else
	{
		// Drop off the water into the hole
		Messages::getInstance().postMessage("PlayerCollectWater");
		GameState::getInstance().playerIsHoldingWater = false;
		GameState::getInstance().removePuddleGUID(guid);
		//GameState::getInstance().requestTriggerRelease(physicsComponent->getActor());

		std::cout << "Dropped water off into Puddle!!!!" << std::endl;
		containsWater = true;
	}
}

void WaterPuddle::preRenderUpdate()
{
	//
	// Process Animations
	//
	if (containsWater)
	{
		animator.playAnimation(0, 1.5f);
	}
	else
	{
		animator.playAnimation(1, 1.5f);
	}
	animator.animationSpeed = 1.0f;
	animator.updateAnimation(MainLoop::getInstance().deltaTime);

	//
	// Do trigger-related stuff
	//
	if (!isBeingTriggeredByPlayer() ||
		getPhysicsComponent()->getActor() != GameState::getInstance().getCurrentTriggerHold())
		return;

	static bool prevEBtnPressed = GLFW_RELEASE;
	bool EBtnpressed = InputManager::getInstance().interactPressed;
	if (prevEBtnPressed == GLFW_RELEASE && EBtnpressed == GLFW_PRESS)
	{
		//
		// Collect this object!
		//
		collectWaterPuddle();
	}
	prevEBtnPressed = EBtnpressed;
}

void WaterPuddle::refreshResources()
{
	bool recreateAnimations;
	model = (Model*)Resources::getResource("model;waterPuddle", model, &recreateAnimations);
	if (recreateAnimations)
		animator = Animator(&model->getAnimations());


	//
	// Materials
	//
	materials["Mound"] = (Material*)Resources::getResource("material;pbrSlimeBelt");
	materials["Water"] = (Material*)Resources::getResource("material;pbrWater");

	model->setMaterials(materials);
}

#ifdef _DEVELOP
void WaterPuddle::imguiPropertyPanel()
{
	ImGui::Checkbox("Contains water", &containsWater);
}

void WaterPuddle::imguiRender()
{
}
#endif
