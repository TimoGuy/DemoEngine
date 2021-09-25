#include "PointLight.h"

#include <glad/glad.h>
#include "../RenderEngine.resources/Resources.h"
#include <glm/gtx/quaternion.hpp>
#include "../../MainLoop/MainLoop.h"

#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_stdlib.h"
#include "../../ImGui/ImGuizmo.h"
#include "../../Utils/PhysicsUtils.h"


PointLight::PointLight()			// TODO: make this in the palette as well, but first, figure out how to implement multiple lights!!!
{
	transform = glm::mat4(1.0f);

	bounds = new Bounds();
	bounds->center = glm::vec3(0.0f);
	bounds->extents = glm::vec3(0.5f);

	imguiComponent = new PointLightImGui(this, bounds);
	lightComponent = new PointLightLight(this);

	lightComponent->getLight().facingDirection = glm::vec3(0.0f);		// 0'd out facingdirection shows it's a point light
}

PointLight::~PointLight()
{
	delete lightComponent;
	delete imguiComponent;
}

void PointLight::loadPropertiesFromJson(nlohmann::json& object)
{
	BaseObject::loadPropertiesFromJson(object["baseObject"]);
	imguiComponent->loadPropertiesFromJson(object["imguiComponent"]);
	lightComponent->loadPropertiesFromJson(object["lightComponent"]);

	//
	// I'll take the leftover tokens then
	//
	lightComponent->getLight().color = glm::vec3(object["color"][0], object["color"][1], object["color"][2]);
	lightComponent->getLight().colorIntensity = object["color_multiplier"];
}

nlohmann::json PointLight::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();
	j["imguiComponent"] = imguiComponent->savePropertiesToJson();
	j["lightComponent"] = lightComponent->savePropertiesToJson();

	j["color"] = { lightComponent->getLight().color.r, lightComponent->getLight().color.g, lightComponent->getLight().color.b };
	j["color_multiplier"] = lightComponent->getLight().colorIntensity;

	return j;
}

PointLightImGui::PointLightImGui(BaseObject* bo, Bounds* bounds) : ImGuiComponent(bo, bounds, "Point Light")			// tODO: create bounds for here
{
	refreshResources();
}

void PointLightImGui::refreshResources()
{
	lightGizmoTextureId = *(GLuint*)Resources::getResource("texture;lightIcon");
}

PointLightLight::PointLightLight(BaseObject* bo) : LightComponent(bo)
{
	light.lightType = LightType::POINT;

	light.color = glm::vec3(1.0f);
	light.colorIntensity = 150.0f;
}

void PointLightImGui::propertyPanelImGui()
{
	ImGui::InputText("Name", &name);
	ImGui::Separator();
	PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(baseObject->transform));
	
	ImGui::ColorEdit3("Light base color", &((PointLight*)baseObject)->lightComponent->getLight().color[0], ImGuiColorEditFlags_DisplayRGB);
	ImGui::DragFloat("Light color multiplier", &((PointLight*)baseObject)->lightComponent->getLight().colorIntensity);
}

void PointLightImGui::renderImGui()
{
#ifdef _DEBUG
	refreshResources();
#endif

	//
	// Draw Light position			(TODO: This needs to get extracted into its own function)
	//
	float gizmoSize1to1 = 30.0f;
	glm::vec3 lightPosOnScreen = MainLoop::getInstance().camera.PositionToClipSpace(PhysicsUtils::getPosition(baseObject->transform));
	float clipZ = lightPosOnScreen.z;


	float gizmoRadius = gizmoSize1to1;
	{
		float a = 0.0f;
		float b = gizmoSize1to1;

		float defaultHeight = std::tanf(glm::radians(MainLoop::getInstance().camera.fov)) * clipZ;
		float t = gizmoSize1to1 / defaultHeight;
		//std::cout << t << std::endl;
		gizmoRadius = t * (b - a) + a;
	}

	if (clipZ > 0.0f)
	{
		lightPosOnScreen /= clipZ;
		lightPosOnScreen.x = lightPosOnScreen.x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
		lightPosOnScreen.y = -lightPosOnScreen.y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;

		//std::cout << lightPosOnScreen.x << ", " << lightPosOnScreen.y << ", " << lightPosOnScreen.z << std::endl;
		ImVec2 p_min = ImVec2(lightPosOnScreen.x - gizmoRadius, lightPosOnScreen.y + gizmoRadius);
		ImVec2 p_max = ImVec2(lightPosOnScreen.x + gizmoRadius, lightPosOnScreen.y - gizmoRadius);

		ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)lightGizmoTextureId, p_min, p_max);
	}

	PhysicsUtils::imguiRenderCircle(baseObject->transform, 0.25f, glm::vec3(0.0f), glm::vec3(0.0f), 16, ImColor::HSV(0.1083f, 0.66f, 0.91f));

	ImGuiComponent::renderImGui();
}

void PointLightImGui::cloneMe()
{
	// TODO: figure this out
}
