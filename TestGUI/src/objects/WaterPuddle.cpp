#include "WaterPuddle.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include "../mainloop/MainLoop.h"
#include "../utils/PhysicsUtils.h"
#include "../utils/GameState.h"
#include "../utils/InputManager.h"
#include "../utils/Messages.h"
#include "components/PhysicsComponents.h"
#include "../render_engine/model/Model.h"
#include "../render_engine/material/Material.h"
#include "../render_engine/resources/Resources.h"
#include "../render_engine/render_manager/RenderManager.h"


WaterPuddle::WaterPuddle() : numWaterServings(1)
{
	name = "Water Puddle";

	physicsComponent = new SphereCollider(this, 4.0f, RigidActorTypes::STATIC, ShapeTypes::TRIGGER);

	refreshResources();
	renderComponent = new RenderComponent(this);
	model->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0, -2.1f, 0));
	renderComponent->addModelToRender({ model, true, &animator });
	waterServingsText = { std::to_string(numWaterServings), getTransform(), glm::vec3(1, 1, 0), TextAlignment::CENTER, TextAlignment::BOTTOM };
	renderComponent->addTextToRender(&waterServingsText);
}

WaterPuddle::~WaterPuddle()
{
	delete renderComponent;
	delete physicsComponent;
}

void WaterPuddle::loadPropertiesFromJson(nlohmann::json& object)
{
	BaseObject::loadPropertiesFromJson(object["baseObject"]);

	if (object.contains("num_water_servings"))
		numWaterServings = object["num_water_servings"];

	refreshResources();

	// Check if I'm already collected
	//if (GameState::getInstance().isPuddleCollected(guid))
	//	containsWater = false;
}

nlohmann::json WaterPuddle::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();

	j["num_water_servings"] = numWaterServings;

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
	MainLoop::getInstance().renderManager->pushMessage("Hello Notif");

	// Event!!!
	if (numWaterServings == 0)
	{
		// @NOTE: This below should be a case of "if numWaterServings >= maxWaterholding" then prevent from filling in more water
		//if (numWaterServings > 0 && GameState::getInstance().playerIsHoldingWater)
		//{
		//	std::cout << "You're already carrying water. Drop if off or drink it to carry more" << std::endl;
		//	return;
		//}

		// Drop off the water into the hole
		if (GameState::getInstance().currentPlayerStaminaAmount - GameState::getInstance().playerStaminaOneServingAmount > 0.0f)
		{
			GameState::getInstance().currentPlayerStaminaAmount -= GameState::getInstance().playerStaminaOneServingAmount;

			Messages::getInstance().postMessage("PlayerCollectWater");
			GameState::getInstance().removePuddleGUID(guid);
			//GameState::getInstance().requestTriggerRelease(physicsComponent->getActor());

			std::cout << "Dropped water off into Puddle!!!!" << std::endl;
			numWaterServings++;
		}
		else
		{
			std::cout << "aborted dopping off water. Not enough water in there." << std::endl;
		}
	}
	else
	{
		if (numWaterServings <= 0)
		{
			std::cout << "This water puddle is empty. Unfortunate." << std::endl;
			return;
		}

		// Collect and log the puddle
		GameState::getInstance().currentPlayerStaminaAmount += GameState::getInstance().playerStaminaOneServingAmount;
		GameState::getInstance().currentPlayerStaminaAmount =
			glm::min(GameState::getInstance().currentPlayerStaminaAmount, (float)GameState::getInstance().maxPlayerStaminaAmount);

		Messages::getInstance().postMessage("PlayerCollectWater");
		GameState::getInstance().addPuddleGUID(guid);
		//GameState::getInstance().requestTriggerRelease(physicsComponent->getActor());

		std::cout << "Collected Water in Puddle!!!!" << std::endl;
		numWaterServings--;
	}
}

void WaterPuddle::preRenderUpdate()
{
	// Update the modelmatrix
	glm::vec3 pos = PhysicsUtils::getPosition(getTransform());
	pos.y += 1.5f;
	waterServingsText.modelMatrix = glm::translate(glm::mat4(1.0f), pos - MainLoop::getInstance().camera.position) * glm::inverse(MainLoop::getInstance().camera.calculateViewMatrix()) * glm::scale(glm::mat4(1.0f), glm::vec3(0.06f));
	waterServingsText.text = std::to_string(numWaterServings);

	//
	// Process Animations
	//
	animatorStateMachine.setVariable("isFull", numWaterServings > 0);
	animatorStateMachine.updateStateMachine(MainLoop::getInstance().deltaTime);

	//
	// Do trigger-related stuff
	//
	if (!isBeingTriggeredByPlayer() ||
		getPhysicsComponent()->getActor() != GameState::getInstance().getCurrentTriggerHold())
		return;

	if (InputManager::getInstance().on_interactPressed)
	{
		//
		// Collect this object!
		//
		collectWaterPuddle();
	}
}

void WaterPuddle::refreshResources()
{
	bool recreateAnimations;
	model = (Model*)Resources::getResource("model;water_puddle", model, &recreateAnimations);
	if (recreateAnimations)
	{
		animator = Animator(&model->getAnimations());
		animatorStateMachine = AnimatorStateMachine("water_puddle", &animator);
	}
}

#ifdef _DEVELOP
void WaterPuddle::imguiPropertyPanel()
{
	ImGui::InputInt("Num Water servings", &numWaterServings);
}

void WaterPuddle::imguiRender()
{
}
#endif
