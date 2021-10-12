#include "WaterPuddle.h"

#include "../MainLoop/MainLoop.h"
#include "../Utils/PhysicsUtils.h"


WaterPuddle::WaterPuddle() : TriggerComponent(this)
{
	bounds = new Bounds();
	bounds->center = glm::vec3(0.0f);
	bounds->extents = glm::vec3(1.0f, 1.0f, 1.0f);

	imguiComponent = new WaterPuddleImgui(this, bounds);
	renderComponent = new WaterPuddleRender(this, bounds);

	//
	// Create Trigger shape
	//
	body = PhysicsUtils::createRigidbodyKinematic(MainLoop::getInstance().physicsPhysics, PhysicsUtils::createTransform(getTransform()));
	triggerShape = MainLoop::getInstance().physicsPhysics->createShape(physx::PxSphereGeometry(4.0f), *MainLoop::getInstance().defaultPhysicsMaterial);
	triggerShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
	triggerShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
	body->attachShape(*triggerShape);
	body->setGlobalPose(PhysicsUtils::createTransform(getTransform()));
	MainLoop::getInstance().physicsScene->addActor(*body);
	triggerShape->release();
}

WaterPuddle::~WaterPuddle()
{
	delete renderComponent;
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

void WaterPuddle::notifyNewTransform(glm::mat4 newTransform)
{
	body->setGlobalPose(PhysicsUtils::createTransform(newTransform));
}

physx::PxActor* WaterPuddle::getActor()
{
	return body;
}

void WaterPuddle::onTrigger(const physx::PxTriggerPair& pair)
{
	std::cout << "Hello, there!!!! " << baseObject->guid << std::endl;
}

WaterPuddleRender::WaterPuddleRender(BaseObject* bo, Bounds* bounds) : RenderComponent(bo, bounds)
{
}

void WaterPuddleRender::preRenderUpdate()
{
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
