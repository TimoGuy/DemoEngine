#include "DirectionalLight.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../mainloop/MainLoop.h"
#include "../render_engine/resources/Resources.h"
#include "../render_engine/render_manager/RenderManager.h"

#define LERP(a, b, t) (a) + (t) * ((b) - (a))
#define REMAP(value, istart, istop, ostart, ostop) ((value) - (istart)) / ((istop) - (istart)) * ((ostop) - (ostart)) + (ostart)

#ifdef _DEVELOP
#include "../imgui/imgui.h"
#include "../imgui/imgui_stdlib.h"
#include "../imgui/ImGuizmo.h"
#endif

#include "../utils/PhysicsUtils.h"

// NOTE: if 0, then use close fit shadows
#define STABLE_FIT_CSM_SHADOWS 1

static float multiplier = 1.0f;				// @Maybe: I think that with the new stable fit csm, we can have multiplier be at 1.0 instead of 2.0. @TODO: Figure out if this is correct or not.
static int followCascade = -1;				// NOTE: this is so that it's off by default

DirectionalLight::DirectionalLight(bool castsShadows)
{
	name = "Directional Light";

	lightComponent = new DirectionalLightLight(this, castsShadows);

	setLookDirection(PhysicsUtils::getRotation(getTransform()));

	refreshResources();
}

DirectionalLight::~DirectionalLight()			// TODO: when there are shadow maps, delete them too! When deleting the light, there is garbage shadow maps
{
	delete lightComponent;
}

void DirectionalLight::loadPropertiesFromJson(nlohmann::json& object)
{
	BaseObject::loadPropertiesFromJson(object["baseObject"]);
	lightComponent->loadPropertiesFromJson(object["lightComponent"]);

	//
	// I'll take the leftover tokens then
	//
	lightComponent->color = glm::vec3(object["color"][0], object["color"][1], object["color"][2]);
	((DirectionalLightLight*)lightComponent)->maxColorIntensity = object["color_multiplier"];

	setLookDirection(PhysicsUtils::getRotation(getTransform()));
	((DirectionalLightLight*)lightComponent)->refreshRenderBuffers();
}

nlohmann::json DirectionalLight::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();
	j["lightComponent"] = lightComponent->savePropertiesToJson();

	j["color"] = { lightComponent->color.r, lightComponent->color.g, lightComponent->color.b };
	j["color_multiplier"] = lightComponent->colorIntensity;

	return j;
}

void DirectionalLight::refreshResources()
{
	lightGizmoTextureId = *(GLuint*)Resources::getResource("texture;lightIcon");
}

DirectionalLightLight::DirectionalLightLight(BaseObject* bo, bool castsShadows) : LightComponent(bo)
{
	LightComponent::castsShadows = castsShadows;

	lightType = LightType::DIRECTIONAL;

	color = glm::vec3(1.0f, 1.0f, 0.984f);		// http://planetpixelemporium.com/tutorialpages/light.html (High Noon Sun)
	maxColorIntensity = 150.0f;
	colorIntensityMaxAtY = 0.707106781f;		// NOTE: sin(45)

	// @Hardcode
	shadowFarPlane = 150.0f;
	//shadowCascadeLevels = { shadowFarPlane * 0.067f, shadowFarPlane * 0.2f, shadowFarPlane * 0.467f };			// This is unity's 4 cascade distribution
	//shadowCascadeLevels = { shadowFarPlane * 0.1f, shadowFarPlane * 0.25f, shadowFarPlane * 0.5f };
	shadowCascadeLevels = { shadowFarPlane * 0.0625f, shadowFarPlane * 0.25f, shadowFarPlane * 0.5625f };			// This one is x^2

	refreshRenderBuffers();
}

DirectionalLightLight::~DirectionalLightLight()
{
	destroyCSMBuffers();
}

void DirectionalLightLight::refreshRenderBuffers()
{
	if (castsShadows)
		createCSMBuffers();
	else
		destroyCSMBuffers();
}

constexpr GLsizei depthMapResolution = 1024;
void DirectionalLightLight::createCSMBuffers()
{
	if (shadowMapsCreated) return;

	refreshResources();

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
		nullptr
	);
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

	// Set flag
	shadowMapsCreated = true;
}

void DirectionalLightLight::destroyCSMBuffers()
{
	if (!shadowMapsCreated) return;

	glDeleteBuffers(1, &matricesUBO);
	glDeleteTextures(1, &shadowMapTexture);
	glDeleteFramebuffers(1, &lightFBO);

	// Set flag
	shadowMapsCreated = false;
}

void DirectionalLightLight::renderPassShadowMap()
{
#ifdef _DEVELOP
	refreshResources();
#endif

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(cascadedShaderProgram);

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
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowMapTexture, 0);
	glViewport(0, 0, depthMapResolution, depthMapResolution);
	glClear(GL_DEPTH_BUFFER_BIT);
	//glCullFace(GL_FRONT);  // peter panning

	// NOTE: enabling depth clamping causes the fragments that are too far behind
	// the clip space to actually just get clamped. This forces things to stay inside
	// the shadowmap when creating it, especially with the CSM
	glEnable(GL_DEPTH_CLAMP);
	MainLoop::getInstance().renderManager->renderSceneShadowPass(cascadedShaderProgram);
	glDisable(GL_DEPTH_CLAMP);

	//glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DirectionalLightLight::refreshResources()
{
	cascadedShaderProgram = *(GLuint*)Resources::getResource("shader;csmShadowPass");
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
				frustumCorners.push_back(corner / corner.w);
			}
		}
	}

	return frustumCorners;
}

bool doThis = false;
float divisor = 1024;
glm::mat4 DirectionalLightLight::getLightSpaceMatrix(const float nearPlane, const float farPlane)
{
	if (MainLoop::getInstance().camera.width == 0.0f || MainLoop::getInstance().camera.height == 0.0f)
		return glm::mat4(1.0f);

	//
	// Getting the light viewproj matrix
	//
	const auto proj = glm::perspective(
		glm::radians(MainLoop::getInstance().camera.fov), MainLoop::getInstance().camera.width / MainLoop::getInstance().camera.height, nearPlane, farPlane
	);
	const auto corners = getFrustumCornersWorldSpace(proj, MainLoop::getInstance().camera.calculateViewMatrix());

	glm::vec3 center = glm::vec3(0, 0, 0);
	for (const auto& v : corners)
	{
		center += glm::vec3(v);
	}
	center /= corners.size();

#if STABLE_FIT_CSM_SHADOWS
	//
	// Stable fit shadows
	//
	float largestDist = std::numeric_limits<float>::min();		// This will be radius of circumscribed circle (http://the-witness.net/news/2010/03/graphics-tech-shadow-maps-part-1/)
	for (const auto& v : corners)
	{
		largestDist = std::max(largestDist, glm::distance(glm::vec3(v), center));
	}
	largestDist = std::ceilf(largestDist);

	const glm::mat4 lightProjection = glm::ortho(-largestDist, largestDist, -largestDist, largestDist, -largestDist * multiplier, largestDist * multiplier);

	// Align center of frustum to texel grid
	const glm::mat3 unstableLightView =
		glm::mat3(
			glm::lookAt(
				center,
				center + facingDirection,
				glm::vec3(0.0f, 1.0f, 0.0f)
			)
		);
	glm::vec3 transformedCenter = unstableLightView * center;

	const float texelSize = largestDist * 2.0f / depthMapResolution;
	transformedCenter.x = std::floorf(transformedCenter.x / texelSize) * texelSize;
	transformedCenter.y = std::floorf(transformedCenter.y / texelSize) * texelSize;
	transformedCenter = glm::inverse(unstableLightView) * transformedCenter;

	const glm::mat4 lightView =
		glm::lookAt(
			transformedCenter,
			transformedCenter + facingDirection,
			glm::vec3(0.0f, 1.0f, 0.0f)
		);

	// @DEBUG: Tanjiro centers himself onto the center of the view frustum (close fit)
	if (doThis)
	{
		doThis = false;
		glm::mat4& trans = MainLoop::getInstance().lightObjects[0]->baseObject->getTransform();
		trans = glm::translate(trans, glm::vec3(glm::vec4(transformedCenter, 1.0f)) - PhysicsUtils::getPosition(trans));
	}
#else
	//
	// Close fit shadows
	//
	const glm::mat4 lightView =
		glm::lookAt(
			center,
			center + facingDirection,
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
	float zMult = multiplier;
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

	std::cout << minX << "," << maxX << "," << minY << "," << maxY << "," << minZ << "," << maxZ << std::endl;

	const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

	// @DEBUG: Tanjiro centers himself onto the center of the view frustum (close fit)
	if (doThis)
	{
		doThis = false;
		glm::mat4& trans = MainLoop::getInstance().lightObjects[0]->baseObject->getTransform();
		trans = glm::translate(trans, glm::vec3(glm::vec4(center, 1.0f)) - PhysicsUtils::getPosition(trans));
	}
#endif

	return lightProjection * lightView;
}

std::vector<glm::mat4> DirectionalLightLight::getLightSpaceMatrices()
{
	std::vector<glm::mat4> ret;
	for (size_t i = 0; i < shadowCascadeLevels.size() + 1; ++i)
	{
		if (followCascade == i)
			doThis = true;
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
			ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1], shadowFarPlane));
		}
	}
	return ret;
}

void DirectionalLight::setLookDirection(glm::quat rotation)
{
	glm::vec3 lookDirection(0.0f, 0.0f, 1.0f);
	lookDirection = rotation * lookDirection;
	lookDirection = glm::normalize(lookDirection);

	lightComponent->facingDirection = lookDirection;
	lightComponent->colorIntensity =
		std::max(
			0.0f,
			(-lookDirection.y > ((DirectionalLightLight*)lightComponent)->colorIntensityMaxAtY) ?
			((DirectionalLightLight*)lightComponent)->maxColorIntensity :
			LERP(0.0f, ((DirectionalLightLight*)lightComponent)->maxColorIntensity, REMAP(-lookDirection.y, 0.0f, ((DirectionalLightLight*)lightComponent)->colorIntensityMaxAtY, 0.0f, 1.0f))
		);
}

#ifdef _DEVELOP
void DirectionalLight::imguiPropertyPanel()
{
	setLookDirection(PhysicsUtils::getRotation(getTransform()));

	ImGui::ColorEdit3("Light base color", &lightComponent->color[0], ImGuiColorEditFlags_DisplayRGB);
	ImGui::DragFloat("Light color multiplier (Max)", &((DirectionalLightLight*)lightComponent)->maxColorIntensity);

	ImGui::DragInt("Cascade Shadow Map Debug", &MainLoop::getInstance().renderManager->debugCSMLayerNum);

	ImGui::InputFloat3("DEBUG", &lightComponent->facingDirection[0]);
	ImGui::DragFloat("Multiplier for shadow", &multiplier, 0.1f, 0.0f, 500.0f);
	ImGui::InputInt("Shadow Cascade center follow", &followCascade);
	ImGui::DragFloat("TEMP DivisorForStableShadows", &divisor);

	static bool plsFollow = false;
	ImGui::Checkbox("Follow ndc mouse pos", &plsFollow);
	if (plsFollow)
	{
		double xpos, ypos;
		glfwGetCursorPos(MainLoop::getInstance().window, &xpos, &ypos);
		xpos /= MainLoop::getInstance().camera.width;
		ypos /= MainLoop::getInstance().camera.height;
		ypos = 1.0 - ypos;
		xpos = xpos * 2.0f - 1.0f;
		ypos = ypos * 2.0f - 1.0f;
		glm::vec3 clipSpacePosition(xpos, ypos, 1.0f);
		glm::vec3 worldSpacePosition = MainLoop::getInstance().camera.clipSpacePositionToWordSpace(clipSpacePosition);

		glm::mat4& trans = MainLoop::getInstance().lightObjects[0]->baseObject->getTransform();
		trans = glm::translate(trans, glm::vec3(glm::vec4(worldSpacePosition, 1.0f)) - PhysicsUtils::getPosition(trans));
	}

	//
	// Define the cascades for the shadows!
	//
	ImGui::Separator();
	ImGui::Text("Shadow Cascade boundaries");
	for (size_t i = 0; i < ((DirectionalLightLight*)getLightComponent())->shadowCascadeLevels.size(); i++)
	{
		ImGui::DragFloat((std::string("Bound ") + std::to_string(i)).c_str(), &((DirectionalLightLight*)getLightComponent())->shadowCascadeLevels[i], 1.0f, i==0?0.0f: ((DirectionalLightLight*)getLightComponent())->shadowCascadeLevels[i-1], i== ((DirectionalLightLight*)getLightComponent())->shadowCascadeLevels.size()-1?0.0f: ((DirectionalLightLight*)getLightComponent())->shadowCascadeLevels[i + 1]);
	}

	//
	// Toggle shadows
	//
	std::string toggleShadowsLabel = "Turn " + std::string(lightComponent->castsShadows ? "Off" : "On") + " Shadows";
	if (ImGui::Button(toggleShadowsLabel.c_str()))
	{
		getLightComponent()->castsShadows = !getLightComponent()->castsShadows;
		((DirectionalLightLight*)getLightComponent())->refreshRenderBuffers();
	}
}

void DirectionalLight::imguiRender()
{
	refreshResources();

	//
	// Draw Light position			(TODO: This needs to get extracted into its own function)
	//
	float gizmoSize1to1 = 30.0f;
	glm::vec3 lightPosOnScreen = MainLoop::getInstance().camera.PositionToClipSpace(PhysicsUtils::getPosition(getTransform()));
	glm::vec3 lightPointingDirection = MainLoop::getInstance().camera.PositionToClipSpace(PhysicsUtils::getPosition(getTransform()) + lightComponent->facingDirection);
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
		lightPosOnScreen.x = ImGui::GetWindowPos().x + lightPosOnScreen.x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
		lightPosOnScreen.y = ImGui::GetWindowPos().y - lightPosOnScreen.y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;

		//std::cout << lightPosOnScreen.x << ", " << lightPosOnScreen.y << ", " << lightPosOnScreen.z << std::endl;
		ImVec2 p_min = ImVec2(lightPosOnScreen.x - gizmoRadius, lightPosOnScreen.y + gizmoRadius);
		ImVec2 p_max = ImVec2(lightPosOnScreen.x + gizmoRadius, lightPosOnScreen.y - gizmoRadius);

		ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)(intptr_t)lightGizmoTextureId, p_min, p_max);
	}

	if (clipZ2 > 0.0f)
	{
		lightPointingDirection /= clipZ2;
		lightPointingDirection.x = ImGui::GetWindowPos().x + lightPointingDirection.x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
		lightPointingDirection.y = ImGui::GetWindowPos().y - lightPointingDirection.y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;

		ImGui::GetBackgroundDrawList()->AddLine(ImVec2(lightPosOnScreen.x, lightPosOnScreen.y), ImVec2(lightPointingDirection.x, lightPointingDirection.y), ImColor::HSV(0.1083f, 0.66f, 0.91f), 3.0f);
	}

	PhysicsUtils::imguiRenderCircle(getTransform(), 0.25f, glm::vec3(0.0f), glm::vec3(0.0f), 16, ImColor::HSV(0.1083f, 0.66f, 0.91f));
}
#endif
