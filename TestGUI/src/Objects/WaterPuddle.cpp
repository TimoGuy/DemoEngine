#include "WaterPuddle.h"

#include "../MainLoop/MainLoop.h"
#include "../Utils/PhysicsUtils.h"
#include "../Utils/GameState.h"
#include "../Utils/Messages.h"
#include "Components/PhysicsComponents.h"
#include "../RenderEngine/RenderEngine.resources/Resources.h"
#include "../RenderEngine/RenderEngine.manager/RenderManager.h"
#include <glm/gtc/type_ptr.hpp>


WaterPuddle::WaterPuddle() : isWaterPuddleCollected(false)
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

WaterPuddleRender::WaterPuddleRender(BaseObject* bo, Bounds* bounds) : RenderComponent(bo, bounds), offsetModelMatrix(glm::translate(glm::mat4(1.0f), glm::vec3(0, -2.1f, 0)))
{
	// Always load up the resources first dawg!
	refreshResources();
}

void WaterPuddleRender::preRenderUpdate()
{
	//
	// Update rendertransform
	//
	renderTransform = baseObject->getTransform() * offsetModelMatrix;

	//
	// Process Animations
	//
	const bool isWaterPuddleCollected = ((WaterPuddle*)baseObject)->isWaterPuddleCollected;
	if (!isWaterPuddleCollected)
	{
		animator.playAnimation(0);
	}
	else
	{
		animator.playAnimation(1, 1.5f);
	}
	animator.updateAnimation(MainLoop::getInstance().deltaTime * 42.0f);

	//
	// Do trigger-related stuff
	//
	if (!((WaterPuddle*)baseObject)->isBeingTriggeredByPlayer() ||
		baseObject->getPhysicsComponent()->getActor() != GameState::getInstance().getCurrentTriggerHold())
		return;

	static bool prevEBtnPressed = GLFW_RELEASE;		// @TODO: have a separate input system that I can query!!! (Or else bugs will ensue if you keep the E button pressed in this example eh
	bool EBtnpressed = glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_E);
	if (prevEBtnPressed == GLFW_RELEASE && EBtnpressed == GLFW_PRESS)
	{
		//
		// Collect this object!
		//
		((WaterPuddle*)baseObject)->collectWaterPuddle();
	}
	prevEBtnPressed = EBtnpressed;
}

void WaterPuddleRender::render()
{
#ifdef _DEBUG
	refreshResources();
#endif

	std::vector<glm::mat4>* boneTransforms = animator.getFinalBoneMatrices();
	MainLoop::getInstance().renderManager->updateSkeletalBonesUBO(*boneTransforms);
	model->render(renderTransform, true);
}

void WaterPuddleRender::renderShadow(GLuint programId)
{
	std::vector<glm::mat4>* boneTransforms = animator.getFinalBoneMatrices();
	MainLoop::getInstance().renderManager->updateSkeletalBonesUBO(*boneTransforms);
	glUniformMatrix4fv(glGetUniformLocation(programId, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(renderTransform));			// @TODO: this shouldn't be here, and model->render should automatically take care of the modelMatrix!
	model->render(renderTransform, false);
}

void WaterPuddleRender::refreshResources()
{
	model = (Model*)Resources::getResource("model;waterPuddle");
	if (!importedAnims)
		animator = Animator(&model->getAnimations());			// NOTE: animations get imported only once!!!!
	importedAnims = true;

	//
	// Materials
	//
	materials["Mound"] = (Material*)Resources::getResource("material;pbrRustyMetal");
	materials["Water"] = (Material*)Resources::getResource("material;pbrSlimeShorts");

	model->setMaterials(materials);
}

void WaterPuddleImgui::propertyPanelImGui()
{
	ImGui::Text("Model Offset ModelMatrix");
	PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(((WaterPuddleRender*)baseObject->getRenderComponent())->getOffsetModelMatrix()));
}

void WaterPuddleImgui::renderImGui()
{
	ImGuiComponent::renderImGui();
}
