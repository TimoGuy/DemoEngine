#include "WaterPuddle.h"

#include "../MainLoop/MainLoop.h"
#include "../Utils/PhysicsUtils.h"
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
