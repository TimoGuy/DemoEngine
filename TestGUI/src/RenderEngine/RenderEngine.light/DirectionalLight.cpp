#include "DirectionalLight.h"

#include <glad/glad.h>
#include "../RenderEngine.resources/TextureResources.h"
#include <glm/gtx/quaternion.hpp>
#include "../../MainLoop/MainLoop.h"

#include "../../ImGui/imgui.h"
#include "../../ImGui/ImGuizmo.h"
#include "../../Utils/PhysicsUtils.h"


DirectionalLight::DirectionalLight(glm::vec3 eulerAngles) : LightComponent(transform), ImGuiComponent((char*)"DirectionalLight", transform)
{
	transform = glm::mat4(1.0f);

	setLookDirection(glm::quat(glm::radians(eulerAngles)));

	lightGizmoTextureId =
		TextureResources::getInstance().loadTexture2D(
			"light_icon", "res/cool_img.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT
		);

	light.lightType = LightType::DIRECTIONAL;

	light.color = glm::vec3(1.0f);
	light.colorIntensity = 150.0f;
}

DirectionalLight::~DirectionalLight()
{
}

void DirectionalLight::setLookDirection(glm::quat rotation)
{
	glm::vec3 lookDirection(0.0f, 0.0f, 1.0f);
	lookDirection = rotation * lookDirection;
	lookDirection = glm::normalize(lookDirection);

	light.facingDirection = lookDirection;
}

void DirectionalLight::propertyPanelImGui()
{
	PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(transform));
	setLookDirection(PhysicsUtils::getRotation(transform));
	
	ImGui::ColorEdit3("Light base color", &light.color[0], ImGuiColorEditFlags_DisplayRGB);
	ImGui::DragFloat("Light color multiplier", &light.colorIntensity);
}

void DirectionalLight::renderImGui()
{
	//
	// Draw Light position			(TODO: This needs to get extracted into its own function)
	//
	float gizmoSize1to1 = 30.0f;
	glm::vec3 lightPosOnScreen = MainLoop::getInstance().camera.PositionToClipSpace(PhysicsUtils::getPosition(transform));
	glm::vec3 lightPointingDirection = MainLoop::getInstance().camera.PositionToClipSpace(PhysicsUtils::getPosition(transform) + light.facingDirection);
	float clipZ = lightPosOnScreen.z;
	float clipZ2 = lightPointingDirection.z;


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

	if (clipZ2 > 0.0f)
	{
		lightPointingDirection /= clipZ2;
		lightPointingDirection.x = lightPointingDirection.x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
		lightPointingDirection.y = -lightPointingDirection.y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;

		ImGui::GetBackgroundDrawList()->AddLine(ImVec2(lightPosOnScreen.x, lightPosOnScreen.y), ImVec2(lightPointingDirection.x, lightPointingDirection.y), ImColor::HSV(0.1083f, 0.66f, 0.91f), 3.0f);
	}
}

void DirectionalLight::cloneMe()
{
	// TODO: figure this out
}
