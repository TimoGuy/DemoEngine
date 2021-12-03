#include "RiverDropoff.h"

#include "../MainLoop/MainLoop.h"
#include "../Utils/PhysicsUtils.h"
#include "../Utils/GameState.h"
#include "../Utils/InputManager.h"
#include "../Utils/Messages.h"
#include "../RenderEngine/RenderEngine.resources/Resources.h"
#include "../RenderEngine/RenderEngine.model/Model.h"
#include "Components/PhysicsComponents.h"


RiverDropoff::RiverDropoff()
{
	name = "River Dropoff Area";

	bounds = new RenderAABB();
	bounds->center = glm::vec3(0.0f);
	bounds->extents = glm::vec3(1.0f, 1.0f, 1.0f);

	physicsComponent = new BoxCollider(this, bounds, RigidActorTypes::STATIC, ShapeTypes::TRIGGER);
	renderComponent = new RiverDropoffRender(this, bounds);
}

RiverDropoff::~RiverDropoff()
{
	delete renderComponent;
	delete physicsComponent;

	delete bounds;
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
	if (!GameState::getInstance().playerIsHoldingWater)
	{
		std::cout << "This is where you drop off water to send off to your sister (make this a textbox)" << std::endl;
		return;
	}

	std::cout << "Dropped Off!!!!" << std::endl;
	Messages::getInstance().postMessage("PlayerCollectWater");
	GameState::getInstance().playerIsHoldingWater = false;
}

RiverDropoffRender::RiverDropoffRender(BaseObject* bo, RenderAABB* bounds) : RenderComponent(bo, bounds)
{
	refreshResources();
}

void RiverDropoffRender::preRenderUpdate()
{
	if (!((RiverDropoff*)baseObject)->isBeingTriggeredByPlayer() ||
		baseObject->getPhysicsComponent()->getActor() != GameState::getInstance().getCurrentTriggerHold())
		return;

	static bool prevEBtnPressed = GLFW_RELEASE;
	bool EBtnpressed = InputManager::getInstance().interactPressed;
	if (prevEBtnPressed == GLFW_RELEASE && EBtnpressed == GLFW_PRESS)
	{
		//
		// Drop off the water!
		//
		((RiverDropoff*)baseObject)->dropoffWater();
	}
	prevEBtnPressed = EBtnpressed;
}

void RiverDropoffRender::render()
{
#ifdef _DEBUG
	refreshResources();
#endif

	model->render(baseObject->getTransform(), true);
}

void RiverDropoffRender::renderShadow(GLuint programId)
{
	glUniformMatrix4fv(glGetUniformLocation(programId, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(renderTransform));			// @TODO: this shouldn't be here, and model->render should automatically take care of the modelMatrix!
	model->render(baseObject->getTransform(), false);
}

void RiverDropoffRender::refreshResources()
{
	model = (Model*)Resources::getResource("model;custommodel;res/debug/trigger_repr_cube.glb");
	materials["Material"] = (Material*)Resources::getResource("material;pbrSlimeVest");
	model->setMaterials(materials);
}

void RiverDropoff::propertyPanelImGui()
{
}

void RiverDropoff::renderImGui()
{
}
