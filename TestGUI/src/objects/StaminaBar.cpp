#include "StaminaBar.h"

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


StaminaBar::StaminaBar()
{
	name = "Stamina Bar (PlayerChar-zoku) (DO NOT DELETE)";

	refreshResources();
	renderComponent = new RenderComponent(this);
	renderComponent->addModelToRender({ model, false, nullptr });
}

StaminaBar::~StaminaBar()
{
	delete renderComponent;
}

void StaminaBar::loadPropertiesFromJson(nlohmann::json& object)
{
	BaseObject::loadPropertiesFromJson(object["baseObject"]);
}

nlohmann::json StaminaBar::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();

	return j;
}

void StaminaBar::preRenderUpdate()
{
	if (!isBeingTriggeredByPlayer() ||
		getPhysicsComponent()->getActor() != GameState::getInstance().getCurrentTriggerHold())
		return;

	static bool prevEBtnPressed = GLFW_RELEASE;
	bool EBtnpressed = InputManager::getInstance().interactPressed;
	if (prevEBtnPressed == GLFW_RELEASE && EBtnpressed == GLFW_PRESS)
	{
		//
		// Drop off the water!
		//
		dropoffWater();
	}
	prevEBtnPressed = EBtnpressed;
}

void StaminaBar::refreshResources()
{
	model = (Model*)Resources::getResource("model;custommodel;res/ui/stamina_meter.glb");
	materials["Material"] = (Material*)Resources::getResource("material;uiStaminaMeter");
	model->setMaterials(materials);
}

#ifdef _DEVELOP
void StaminaBar::imguiPropertyPanel()
{
}

void StaminaBar::imguiRender()
{
}
#endif
