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


WaterPuddle::WaterPuddle() : isWaterPuddleCollected(false)
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

	//
	// I'll take the leftover tokens then
	//
}

nlohmann::json WaterPuddle::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();

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
	if (isWaterPuddleCollected)
	{
		std::cout << "This water puddle is already collected. Unfortunate." << std::endl;
		return;
	}

	if (GameState::getInstance().playerIsHoldingWater)
	{
		std::cout << "You're already carrying water. Drop if off or drink it to carry more" << std::endl;
		return;
	}

	//
	// Collect and log the puddle
	//
	Messages::getInstance().postMessage("PlayerCollectWater");
	GameState::getInstance().playerIsHoldingWater = true;
	GameState::getInstance().playerAllCollectedPuddleGUIDs.push_back(guid);
	GameState::getInstance().requestTriggerRelease(physicsComponent->getActor());

	std::cout << "Collected!!!!" << std::endl;
	isWaterPuddleCollected = true;
}

void WaterPuddle::preRenderUpdate()
{
	//
	// Process Animations
	//
	if (!isWaterPuddleCollected)
	{
		animator.playAnimation(0);
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
	materials["Mound"] = (Material*)Resources::getResource("material;pbrRustyMetal");
	materials["Water"] = (Material*)Resources::getResource("material;pbrSlimeShorts");

	model->setMaterials(materials);
}

#ifdef _DEBUG
void WaterPuddle::imguiPropertyPanel()
{
	//ImGui::Text("Model Offset ModelMatrix");
	//PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(((WaterPuddleRender*)renderComponent)->getOffsetModelMatrix()));
}

void WaterPuddle::imguiRender()
{
}
#endif
