#include "DirectionalLight.h"

#include <glad/glad.h>
#include "../RenderEngine.resources/Resources.h"
#include <glm/gtx/quaternion.hpp>
#include "../../MainLoop/MainLoop.h"
#include "../RenderEngine.manager/RenderManager.h"

#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_stdlib.h"
#include "../../ImGui/ImGuizmo.h"
#include "../../Utils/PhysicsUtils.h"
#include <glm/glm.hpp>e


DirectionalLight::DirectionalLight(bool castsShadows)
{
	transform = glm::mat4(1.0f);

	imguiComponent = new DirectionalLightImGui(this);
	lightComponent = new DirectionalLightLight(this, castsShadows);

	setLookDirection(PhysicsUtils::getRotation(transform));
}

DirectionalLight::~DirectionalLight()
{
	delete lightComponent;
	delete imguiComponent;
}

DirectionalLightImGui::DirectionalLightImGui(BaseObject* bo) : ImGuiComponent(bo, "Directional Light")
{
	lightGizmoTextureId = *(GLuint*)Resources::getResource("texture;lightIcon");
}

DirectionalLightLight::DirectionalLightLight(BaseObject* bo, bool castsShadows) : LightComponent(bo)
{
	LightComponent::castsShadows = castsShadows;
	if (castsShadows)
		createCSMBuffers();

	light.lightType = LightType::DIRECTIONAL;

	light.color = glm::vec3(1.0f);
	light.colorIntensity = 150.0f;
}

constexpr unsigned int depthMapResolution = 4096;
void DirectionalLightLight::createCSMBuffers()
{
	cascadedShaderProgram = *(GLuint*)Resources::getResource("shader;csmShadowPass");

	float cameraFarPlane = MainLoop::getInstance().camera.zFar;
	shadowCascadeLevels = { cameraFarPlane / 50.0f, cameraFarPlane / 25.0f, cameraFarPlane / 10.0f, cameraFarPlane / 2.0f };

	//
	// Create light FBO
	//
	glGenFramebuffers(1, &lightFBO);

	glGenTextures(1, &shadowMapTexture);
	glBindTexture(GL_TEXTURE_2D_ARRAY, shadowMapTexture);
	glTexImage3D(
		GL_TEXTURE_2D_ARRAY,
		0,
		GL_DEPTH_COMPONENT32F,
		depthMapResolution,
		depthMapResolution,
		int(shadowCascadeLevels.size()) + 1,
		0,
		GL_DEPTH_COMPONENT,
		GL_FLOAT,
		nullptr);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);

	glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!";
		throw 0;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//
	// Create UBO
	//
	glGenBuffers(1, &matricesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4x4) * 16, nullptr, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void DirectionalLightLight::renderPassShadowMap()
{
	glUseProgram(cascadedShaderProgram);
	glUniform1f(glGetUniformLocation(cascadedShaderProgram, "farPlane"), MainLoop::getInstance().camera.zFar);

	// Setup UBOs
	const auto lightMatrices = getLightSpaceMatrices();
	glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
	for (size_t i = 0; i < lightMatrices.size(); i++)
	{
		glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4x4), sizeof(glm::mat4x4), &lightMatrices[i]);
	}
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// Render depth of scene
	glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_TEXTURE_2D_ARRAY, shadowMapTexture, 0);
	glViewport(0, 0, depthMapResolution, depthMapResolution);
	glClear(GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_FRONT);  // peter panning

	MainLoop::getInstance().renderManager->renderSceneShadowPass(cascadedShaderProgram);

	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::vector<glm::vec4> DirectionalLightLight::getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
{
	const auto inverseMatrix = glm::inverse(proj * view);

	std::vector<glm::vec4> frustumCorners;
	for (unsigned int x = 0; x < 2; x++)
	{
		for (unsigned int y = 0; y < 2; y++)
		{
			for (unsigned int z = 0; z < 2; z++)
			{
				const glm::vec4 corner =
					inverseMatrix * glm::vec4(
						2.0f * x - 1.0f,
						2.0f * y - 1.0f,
						2.0f * z - 1.0f,
						1.0f
					);
				frustumCorners.push_back(corner);
			}
		}
	}

	return frustumCorners;
}


glm::mat4 DirectionalLightLight::getLightSpaceMatrix(const float nearPlane, const float farPlane)
{
	//
	// Getting the light viewproj matrix
	//
	const auto proj = glm::perspective(
		glm::radians(MainLoop::getInstance().camera.fov), MainLoop::getInstance().camera.width / MainLoop::getInstance().camera.height, nearPlane,
		farPlane);
	const auto corners = getFrustumCornersWorldSpace(proj, MainLoop::getInstance().camera.calculateViewMatrix());

	glm::vec3 center = glm::vec3(0, 0, 0);
	for (const auto& v : corners)
	{
		center += glm::vec3(v);
	}
	center /= corners.size();

	const auto lightView =
		glm::lookAt(
			center + getLight().facingDirection,
			center,
			glm::vec3(0.0f, 1.0f, 0.0f)
		);

	float minX = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::min();
	float minY = std::numeric_limits<float>::max();
	float maxY = std::numeric_limits<float>::min();
	float minZ = std::numeric_limits<float>::max();
	float maxZ = std::numeric_limits<float>::min();
	for (const auto& v : corners)
	{
		const auto trf = lightView * v;
		minX = std::min(minX, trf.x);
		maxX = std::max(maxX, trf.x);
		minY = std::min(minY, trf.y);
		maxY = std::max(maxY, trf.y);
		minZ = std::min(minZ, trf.z);
		maxZ = std::max(maxZ, trf.z);
	}

	// Tune this parameter according to the scene
	constexpr float zMult = 10.0f;
	if (minZ < 0)
	{
		minZ *= zMult;
	}
	else
	{
		minZ /= zMult;
	}
	if (maxZ < 0)
	{
		maxZ /= zMult;
	}
	else
	{
		maxZ *= zMult;
	}

	const glm::mat4 lpMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, minZ, maxZ);

	const float scaleX = 2.0f / (maxX - minX);
	const float scaleY = 2.0f / (maxY - minY);
	const float offsetX = -0.5f * (minX + maxX) * scaleX;
	const float offsetY = -0.5f * (minY + maxY) * scaleY;

	glm::mat4 cropMatrix(1.0f);
	cropMatrix[0][0] = scaleX;
	cropMatrix[1][1] = scaleY;
	cropMatrix[3][0] = offsetX;
	cropMatrix[3][1] = offsetY;

	return cropMatrix * lpMatrix * lightView;
}

std::vector<glm::mat4> DirectionalLightLight::getLightSpaceMatrices()
{
	std::vector<glm::mat4> ret;
	for (size_t i = 0; i < shadowCascadeLevels.size() + 1; ++i)
	{
		if (i == 0)
		{
			ret.push_back(getLightSpaceMatrix(MainLoop::getInstance().camera.zNear, shadowCascadeLevels[i]));
		}
		else if (i < shadowCascadeLevels.size())
		{
			ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1], shadowCascadeLevels[i]));
		}
		else
		{
			ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1], MainLoop::getInstance().camera.zFar));
		}
	}
	return ret;
}

void DirectionalLight::setLookDirection(glm::quat rotation)
{
	glm::vec3 lookDirection(0.0f, 0.0f, 1.0f);
	lookDirection = rotation * lookDirection;
	lookDirection = glm::normalize(lookDirection);

	lightComponent->getLight().facingDirection = lookDirection;
}

void DirectionalLightImGui::propertyPanelImGui()
{
	ImGui::InputText("Name", &name);
	ImGui::Separator();
	PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(baseObject->transform));
	((DirectionalLight*)baseObject)->setLookDirection(PhysicsUtils::getRotation(baseObject->transform));
	
	ImGui::ColorEdit3("Light base color", &((DirectionalLight*)baseObject)->lightComponent->getLight().color[0], ImGuiColorEditFlags_DisplayRGB);
	ImGui::DragFloat("Light color multiplier", &((DirectionalLight*)baseObject)->lightComponent->getLight().colorIntensity);

	ImGui::DragInt("Cascade Shadow Map Debug", &MainLoop::getInstance().renderManager->debugNum);
}

void DirectionalLightImGui::renderImGui()
{
	//
	// Draw Light position			(TODO: This needs to get extracted into its own function)
	//
	float gizmoSize1to1 = 30.0f;
	glm::vec3 lightPosOnScreen = MainLoop::getInstance().camera.PositionToClipSpace(PhysicsUtils::getPosition(baseObject->transform));
	glm::vec3 lightPointingDirection = MainLoop::getInstance().camera.PositionToClipSpace(PhysicsUtils::getPosition(baseObject->transform) + ((DirectionalLight*)baseObject)->lightComponent->getLight().facingDirection);
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

	PhysicsUtils::imguiRenderCircle(baseObject->transform, 0.25f, glm::vec3(0.0f), glm::vec3(0.0f), 16, ImColor::HSV(0.1083f, 0.66f, 0.91f));
}

void DirectionalLightImGui::cloneMe()
{
	// TODO: figure this out
}
