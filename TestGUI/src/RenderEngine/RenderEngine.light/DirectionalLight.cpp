#include "DirectionalLight.h"

#include <glad/glad.h>
#include "../RenderEngine.resources/TextureResources.h"
#include <glm/gtx/quaternion.hpp>

#include "../../ImGui/imgui.h"


DirectionalLight::DirectionalLight(glm::vec3 eulerAngles)
{
	setLookDirection(eulerAngles);

	lightGizmoTextureId =
		TextureResources::getInstance().loadTexture2D(
			"light_icon", "res/cool_img.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT
		);
}

void DirectionalLight::setLookDirection(glm::vec3 eulerAngles)
{
	glm::vec3 lookDirection(0.0f, 0.0f, 1.0f);
	lookDirection = glm::quat(glm::vec3(glm::radians(eulerAngles.x), glm::radians(eulerAngles.y), glm::radians(eulerAngles.z))) * lookDirection;
	lookDirection = glm::normalize(lookDirection);

	light.facingDirection = lookDirection;
}

void DirectionalLight::renderImGui(Camera& camera)
{
	//
	// Draw Light position			(TODO: This needs to get extracted into its own function)
	//
	float gizmoSize1to1 = 30.0f;
	glm::vec3 lightPosOnScreen = camera.PositionToClipSpace(light.position);
	glm::vec3 lightPointingDirection = camera.PositionToClipSpace(light.position + light.facingDirection);
	float clipZ = lightPosOnScreen.z;


	float gizmoRadius = gizmoSize1to1;
	{
		float a = 0.0f;
		float b = gizmoSize1to1;

		float defaultHeight = std::tanf(glm::radians(camera.fov)) * clipZ;
		float t = gizmoSize1to1 / defaultHeight;
		//std::cout << t << std::endl;
		gizmoRadius = t * (b - a) + a;
	}

	if (clipZ > 0.0f)
	{
		lightPosOnScreen /= clipZ;
		lightPosOnScreen.x = lightPosOnScreen.x * camera.width / 2 + camera.width / 2;
		lightPosOnScreen.y = -lightPosOnScreen.y * camera.height / 2 + camera.height / 2;
		//std::cout << lightPosOnScreen.x << ", " << lightPosOnScreen.y << ", " << lightPosOnScreen.z << std::endl;
		ImVec2 p_min = ImVec2(lightPosOnScreen.x - gizmoRadius, lightPosOnScreen.y + gizmoRadius);
		ImVec2 p_max = ImVec2(lightPosOnScreen.x + gizmoRadius, lightPosOnScreen.y - gizmoRadius);

		ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)lightGizmoTextureId, p_min, p_max);
		ImGui::GetBackgroundDrawList()->AddLine(ImVec2(lightPosOnScreen.x, lightPosOnScreen.y), ImVec2(lightPointingDirection.x, lightPointingDirection.y), ImColor::HSV(39.0f, 66.0f, 91.0f));
	}
}