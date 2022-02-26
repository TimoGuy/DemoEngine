#include "DirectionalLight.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../mainloop/MainLoop.h"
#include "../render_engine/material/Texture.h"
#include "../render_engine/resources/Resources.h"
#include "../render_engine/render_manager/RenderManager.h"
#include "../render_engine/material/Shader.h"
#include "../utils/GameState.h"

#define LERP(a, b, t) (a) + (t) * ((b) - (a))
#define REMAP(value, istart, istop, ostart, ostop) ((value) - (istart)) / ((istop) - (istart)) * ((ostop) - (ostart)) + (ostart)

#ifdef _DEVELOP
#include "../utils/InputManager.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_stdlib.h"
#include "../imgui/ImGuizmo.h"
#endif

#include "../utils/PhysicsUtils.h"

// NOTE: if 0, then use close fit shadows; if 1, then use stable fit
// NOTE: So bc there's a day night cycle, I think it'd be better if we used close fit. -Timo (2022-01-09)
// RESPONSE: Hmmm, so after looking at close fit shadows swimming again, I think stable fit is still better... umm, but @TODO look at this again. -Timo (2022-01-09, 2:54p)
// NOTE: Keep in mind that the stable-fit shadows don't have texel size understanding (I think). So aligning the texels of the depthmap onto the correct area requires a different formula for rounding the texels.  -Timo (2022-02-25, 4:09am)
// ALSO: We're switching to close-fit shadows. Turns out there was a very weird issue with the close fit shadows that made them look ugly. After figuring out the z axis in light space was backwards (z * -1), the shadows look way better. With a day-night cycle and constant shadow changing, the shadow will swim even if it were stable. Thus, doing close-fit shadows are the wisest. Let's switch to close-fit. More shadow size efficiency.  -Timo (2022-02-25, 4:12am)
#define STABLE_FIT_CSM_SHADOWS 0

#if STABLE_FIT_CSM_SHADOWS
static float multiplier = 1.0f;				// @Maybe: I think that with the new stable fit csm, we can have multiplier be at 1.0 instead of 2.0. @TODO: Figure out if this is correct or not.
static float planeOffset = 0.0f;
#else
static float multiplier = 2.0f;
static float planeOffset = 3.0f;
#endif

DirectionalLight::DirectionalLight(bool castsShadows)
{
	name = "Directional Light";

	hirumaColor = glm::vec3(1.0f, 0.933f, 0.890f);		// https://andi-siess.de/rgb-to-color-temperature/ (5600k light)
	hiokureColor = glm::vec3(1.0f, 0.722f, 0.447f);		// https://andi-siess.de/rgb-to-color-temperature/ (3100k light)

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
	if (object.contains("hiruma_color") && object.contains("hiokure_color"))
	{
		hirumaColor = glm::vec3(object["hiruma_color"][0], object["hiruma_color"][1], object["hiruma_color"][2]);
		hiokureColor = glm::vec3(object["hiokure_color"][0], object["hiokure_color"][1], object["hiokure_color"][2]);
	}
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

	j["hiruma_color"] = { hirumaColor.r, hirumaColor.g, hirumaColor.b };
	j["hiokure_color"] = { hiokureColor.r, hiokureColor.g, hiokureColor.b };
	j["color_multiplier"] = ((DirectionalLightLight*)lightComponent)->maxColorIntensity;

	return j;
}

void DirectionalLight::refreshResources()
{
	lightGizmoTextureId = ((Texture*)Resources::getResource("texture;lightIcon"))->getHandle();
}

DirectionalLightLight::DirectionalLightLight(BaseObject* bo, bool castsShadows) : LightComponent(bo)
{
	LightComponent::castsShadows = castsShadows;

	lightType = LightType::DIRECTIONAL;

	maxColorIntensity = 10.0f;
	colorIntensityMaxAtY = 0.707106781f;		// NOTE: sin(45)

	// @Hardcode
	shadowFarPlane = 250.0f;  //150.0f;
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

constexpr GLsizei depthMapResolution = 1024;		// @Settings: You can change this to 2048 or 4096!
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

	csmShader->use();

	// Setup UBOs
	const std::vector<glm::mat4> lightMatrices = getLightSpaceMatrices();
	glNamedBufferSubData(matricesUBO, 0, sizeof(glm::mat4x4) * lightMatrices.size(), &lightMatrices[0]);

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
	MainLoop::getInstance().renderManager->renderSceneShadowPass(csmShader);
	glDisable(GL_DEPTH_CLAMP);

	//glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DirectionalLightLight::refreshResources()
{
	csmShader = (Shader*)Resources::getResource("shader;csmShadowPass");
}

std::vector<glm::vec4> DirectionalLightLight::getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
{
	const auto inverseMatrix = glm::inverse(proj * view);

	std::vector<glm::vec4> frustumCorners;
	for (int x = 0; x < 2; x++)
	{
		for (int y = 0; y < 2; y++)
		{
			for (int z = 0; z < 2; z++)
			{
				const glm::vec4 corner =
					inverseMatrix * glm::vec4(
						2.0f * (float)x - 1.0f,
						2.0f * (float)y - 1.0f,
						2.0f * (float)z - 1.0f,
						1.0f
					);
				frustumCorners.push_back(corner / corner.w);
			}
		}
	}

	return frustumCorners;
}

#ifdef _DEVELOP
struct DEBUG_frustumLightSpaceCalculations
{
	std::vector<glm::vec3> lightSpaceCenters;
	std::vector<glm::vec3> frustumCornersLightSpace;
	std::vector<glm::vec3> frustumCornersViewSpace;
	std::vector<glm::vec3> frustumCornersJojoSpace;
};

std::vector<DEBUG_frustumLightSpaceCalculations> heyho;
#endif

glm::mat4 DirectionalLightLight::getLightSpaceMatrix(const float nearPlane, const float farPlane)
{
	if (MainLoop::getInstance().camera.width == 0.0f || MainLoop::getInstance().camera.height == 0.0f)
		return glm::mat4(1.0f);

	//
	// Getting the light viewproj matrix
	//
	const auto proj = glm::perspective(
		glm::radians(MainLoop::getInstance().camera.fov),
		MainLoop::getInstance().camera.width / MainLoop::getInstance().camera.height,
		nearPlane,
		farPlane
	);
	const auto corners = getFrustumCornersWorldSpace(proj, MainLoop::getInstance().camera.calculateViewMatrix());

	glm::vec3 center(0.0f);
	for (const auto& v : corners)
	{
		center += glm::vec3(v);
	}
	center /= (float)corners.size();

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
#else
	//
	// Close fit shadows
	//
	const glm::mat4 lightView =
		glm::lookAt(
			center - glm::normalize(facingDirection),
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
		minZ = std::min(minZ, -trf.z);
		maxZ = std::max(maxZ, -trf.z);
	}

	// Tune this parameter according to the scene
	//float zMult = 1.0f;  // multiplier;
	//if (minZ < 0)
	//{
	//	minZ *= zMult;
	//}
	//else
	//{
	//	minZ /= zMult;
	//}
	//if (maxZ < 0)
	//{
	//	maxZ /= zMult;
	//}
	//else
	//{
	//	maxZ *= zMult;
	//}

	float texelSize = maxX - minX;
	if (maxY - minY > texelSize)
		texelSize = maxY - minY;
	texelSize = 1.0f / (float)(depthMapResolution + texelSize);
	shadowCascadeTexelSizes.push_back(texelSize);

	const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

	if (InputManager::getInstance().pausePressed)
	{
		DEBUG_frustumLightSpaceCalculations& newJ = heyho[heyho.size() - 1];
		newJ.lightSpaceCenters.push_back(center);

		const auto cornersLightFrustum = getFrustumCornersWorldSpace(lightProjection, lightView);
		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[0]);
		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[4]);

		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[1]);
		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[5]);

		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[0]);
		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[1]);

		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[4]);
		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[5]);

		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[0]);
		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[2]);

		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[1]);
		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[3]);

		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[4]);
		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[6]);

		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[5]);
		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[7]);

		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[2]);
		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[3]);

		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[6]);
		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[7]);

		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[2]);
		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[6]);

		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[3]);
		newJ.frustumCornersLightSpace.push_back(cornersLightFrustum[7]);


		//for (size_t i = 0; i < cornersLightFrustum.size(); i++)
		//{
		//	frustumCornersLightSpace.push_back(cornersLightFrustum[i]);
		//	frustumCornersLightSpace.push_back(cornersLightFrustum[glm::mod((float)i + 1.0f, (float)cornersLightFrustum.size())]);
		//}

		//for (size_t i = 0; i < corners.size(); i++)
		//{
		//	frustumCornersViewSpace.push_back(corners[i]);
		//	frustumCornersViewSpace.push_back(corners[glm::mod((float)i + 1.0f, (float)corners.size())]);
		//}
		newJ.frustumCornersViewSpace.push_back(corners[0]);
		newJ.frustumCornersViewSpace.push_back(corners[4]);

		newJ.frustumCornersViewSpace.push_back(corners[1]);
		newJ.frustumCornersViewSpace.push_back(corners[5]);

		newJ.frustumCornersViewSpace.push_back(corners[0]);
		newJ.frustumCornersViewSpace.push_back(corners[1]);

		newJ.frustumCornersViewSpace.push_back(corners[4]);
		newJ.frustumCornersViewSpace.push_back(corners[5]);

		newJ.frustumCornersViewSpace.push_back(corners[0]);
		newJ.frustumCornersViewSpace.push_back(corners[2]);

		newJ.frustumCornersViewSpace.push_back(corners[1]);
		newJ.frustumCornersViewSpace.push_back(corners[3]);

		newJ.frustumCornersViewSpace.push_back(corners[4]);
		newJ.frustumCornersViewSpace.push_back(corners[6]);

		newJ.frustumCornersViewSpace.push_back(corners[5]);
		newJ.frustumCornersViewSpace.push_back(corners[7]);

		newJ.frustumCornersViewSpace.push_back(corners[2]);
		newJ.frustumCornersViewSpace.push_back(corners[3]);

		newJ.frustumCornersViewSpace.push_back(corners[6]);
		newJ.frustumCornersViewSpace.push_back(corners[7]);

		newJ.frustumCornersViewSpace.push_back(corners[2]);
		newJ.frustumCornersViewSpace.push_back(corners[6]);

		newJ.frustumCornersViewSpace.push_back(corners[3]);
		newJ.frustumCornersViewSpace.push_back(corners[7]);

		// HEY THERES JOJO SPACE!!! 
		newJ.frustumCornersJojoSpace.push_back(lightView * corners[0]);
		newJ.frustumCornersJojoSpace.push_back(lightView * corners[4]);

		newJ.frustumCornersJojoSpace.push_back(lightView * corners[1]);
		newJ.frustumCornersJojoSpace.push_back(lightView * corners[5]);

		newJ.frustumCornersJojoSpace.push_back(lightView * corners[0]);
		newJ.frustumCornersJojoSpace.push_back(lightView * corners[1]);

		newJ.frustumCornersJojoSpace.push_back(lightView * corners[4]);
		newJ.frustumCornersJojoSpace.push_back(lightView * corners[5]);

		newJ.frustumCornersJojoSpace.push_back(lightView * corners[0]);
		newJ.frustumCornersJojoSpace.push_back(lightView * corners[2]);

		newJ.frustumCornersJojoSpace.push_back(lightView * corners[1]);
		newJ.frustumCornersJojoSpace.push_back(lightView * corners[3]);

		newJ.frustumCornersJojoSpace.push_back(lightView * corners[4]);
		newJ.frustumCornersJojoSpace.push_back(lightView * corners[6]);

		newJ.frustumCornersJojoSpace.push_back(lightView * corners[5]);
		newJ.frustumCornersJojoSpace.push_back(lightView * corners[7]);

		newJ.frustumCornersJojoSpace.push_back(lightView * corners[2]);
		newJ.frustumCornersJojoSpace.push_back(lightView * corners[3]);

		newJ.frustumCornersJojoSpace.push_back(lightView * corners[6]);
		newJ.frustumCornersJojoSpace.push_back(lightView * corners[7]);

		newJ.frustumCornersJojoSpace.push_back(lightView * corners[2]);
		newJ.frustumCornersJojoSpace.push_back(lightView * corners[6]);

		newJ.frustumCornersJojoSpace.push_back(lightView * corners[3]);
		newJ.frustumCornersJojoSpace.push_back(lightView * corners[7]);
	}

#endif

	return lightProjection * lightView;
}

std::vector<glm::mat4> DirectionalLightLight::getLightSpaceMatrices()
{
	if (InputManager::getInstance().pausePressed)
		heyho.push_back(DEBUG_frustumLightSpaceCalculations());

	shadowCascadeTexelSizes.clear();

	std::vector<glm::mat4> ret;
	for (size_t i = 0; i < shadowCascadeLevels.size() + 1; ++i)
	{
		if (i == 0)
		{
			ret.push_back(getLightSpaceMatrix(MainLoop::getInstance().camera.zNear, shadowCascadeLevels[i] + planeOffset));
		}
		else if (i < shadowCascadeLevels.size())
		{
			ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1] - planeOffset, shadowCascadeLevels[i] + planeOffset));
		}
		else
		{
			ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1] - planeOffset, shadowFarPlane));
		}
	}
	return ret;
}

float shadowDisappearMultiplier = 2.14524f;		// @Tuned value
float shadowDisappearOffset = 1.0f;		// @Tuned value
float overflowYAmountRaw;
float startDimmingY = -0.45f;		// @Tuned value
void DirectionalLight::setLookDirection(glm::quat rotation)
{
	glm::vec3 sunOrientation(0.0f, 0.0f, 1.0f);
	sunOrientation = rotation * sunOrientation;
	sunOrientation = glm::normalize(sunOrientation);

	// Modify the light direction
	glm::vec3 lightDirection = sunOrientation;
	lightDirection.y -= 0.2f;			// @NOTE: This seems to be the best one. There's a bit of shadow acne, however.
	lightDirection = glm::normalize(lightDirection);

	// Color intensity
	overflowYAmountRaw = (startDimmingY - lightDirection.y);
	float overflowYAmount = glm::clamp(overflowYAmountRaw * shadowDisappearMultiplier + shadowDisappearOffset, 0.0f, 1.0f);		// Man I wish I could show you the desmos graph I had to make to get this to work the way I wanted. -Timo
	overflowYAmount = glm::pow(overflowYAmount, 2.0f);

	const float clampDirectionY = -0.225f;		// @Tuned value
	if (lightDirection.y > clampDirectionY)
	{
		glm::vec3 lightDirXZ(lightDirection.x, 0, lightDirection.z);
		float clampYAsin = glm::asin(clampDirectionY);
		lightDirXZ = glm::normalize(lightDirXZ) * glm::cos(clampYAsin);

		lightDirection = lightDirXZ;
		lightDirection.y = clampDirectionY;
	}

	lightComponent->facingDirection = lightDirection;
	lightComponent->color = PhysicsUtils::lerp(hiokureColor, hirumaColor, glm::vec3(glm::clamp(-lightComponent->facingDirection.y, 0.0f, 1.0f)));
	lightComponent->colorIntensity = glm::clamp(((DirectionalLightLight*)lightComponent)->maxColorIntensity * glm::pow(overflowYAmount, 2.0f), 0.0f, ((DirectionalLightLight*)lightComponent)->maxColorIntensity);
	
	constexpr float b = -12.0f;		// @Tuned value
	constexpr float c = -b + 1.0f;		// @Tuned value
	MainLoop::getInstance().renderManager->setVolumetricLightingStrength(b * glm::pow(overflowYAmount, 2.0f) + c);
	MainLoop::getInstance().renderManager->getSkyboxParams().sunOrientation = sunOrientation;
	MainLoop::getInstance().renderManager->getSkyboxParams().sunAlpha = sunOrientation.y > 0.0f ? 0.0f : glm::pow(glm::abs(sunOrientation.y), 0.8f);
}

void DirectionalLight::preRenderUpdate()
{
	const float angle = PhysicsUtils::lerp(0.0f, 180.0f, GameState::getInstance().dayNightTime);
	setLookDirection(glm::quat(glm::radians(glm::vec3(angle, -75.0f, 15.0f))));
	MainLoop::getInstance().renderManager->getSkyboxParams().nightSkyTransform = glm::toMat3(glm::quat(glm::radians(glm::vec3(0.0f, 0.0f, -angle / 1.5f))));
}

#ifdef _DEVELOP
int heyhoIndex = 0;
void DirectionalLight::imguiPropertyPanel()
{
	ImGui::ColorEdit3("Light base color (Hiruma)", &hirumaColor[0], ImGuiColorEditFlags_DisplayRGB);
	ImGui::ColorEdit3("Light base color (Hiokure)", &hiokureColor[0], ImGuiColorEditFlags_DisplayRGB);
	ImGui::DragFloat("Light color multiplier (Max)", &((DirectionalLightLight*)lightComponent)->maxColorIntensity);

	ImGui::DragInt("Cascade Shadow Map Debug", &MainLoop::getInstance().renderManager->debugCSMLayerNum);

	ImGui::InputFloat3("DEBUG", &lightComponent->facingDirection[0]);
	ImGui::DragFloat("Multiplier for shadow", &multiplier, 0.1f, 0.0f, 500.0f);
	ImGui::DragFloat("Offset for shadow bounds", &planeOffset, 0.05f, 0.0f, 500.0f);

	//static bool plsFollow = false;
	//ImGui::Checkbox("Follow ndc mouse pos", &plsFollow);
	//if (plsFollow)
	//{
	//	double xpos, ypos;
	//	glfwGetCursorPos(MainLoop::getInstance().window, &xpos, &ypos);
	//	xpos /= MainLoop::getInstance().camera.width;
	//	ypos /= MainLoop::getInstance().camera.height;
	//	ypos = 1.0 - ypos;
	//	xpos = xpos * 2.0f - 1.0f;
	//	ypos = ypos * 2.0f - 1.0f;
	//	glm::vec3 clipSpacePosition(xpos, ypos, 1.0f);
	//	glm::vec3 worldSpacePosition = MainLoop::getInstance().camera.clipSpacePositionToWordSpace(clipSpacePosition);
	//
	//	glm::mat4& trans = MainLoop::getInstance().lightObjects[0]->baseObject->getTransform();
	//	trans = glm::translate(trans, glm::vec3(glm::vec4(worldSpacePosition, 1.0f)) - PhysicsUtils::getPosition(trans));
	//}

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

	//
	// Day night cycle
	//
	ImGui::Separator();
	ImGui::Text("Day night cycle");		// @NOTE: set time of day to 0.075 for sunrise, -0.0793005 for when the offsetY happens
	ImGui::DragFloat("Start Dimming Y", &startDimmingY);
	ImGui::DragFloat("Shadow Disappear mult", &shadowDisappearMultiplier);
	ImGui::DragFloat("Shadow Disappear offset", &shadowDisappearOffset);

	ImGui::Text(("lightDirection and Clamp Diff: " + std::to_string(overflowYAmountRaw)).c_str());
	float jjjj = glm::clamp(overflowYAmountRaw * shadowDisappearMultiplier + shadowDisappearOffset, 0.0f, 1.0f);
	ImGui::Text(("Evaluated: " + std::to_string(jjjj)).c_str());

	ImGui::Separator();
	ImGui::DragInt("HEYHO jerryo", &heyhoIndex, 1.0f, 0, heyho.size());
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

	//
	// @COPYPASTA: render the bounding boxes of the shadows
	//
	if (heyho.size() == 0)
		return;

	int currentIndex = glm::clamp(0, (int)heyho.size() - 1, heyhoIndex);
	for (size_t i = 1; i < heyho[currentIndex].frustumCornersLightSpace.size(); i+=2)
	{
		if (i > 24)		// Only render the first frustum
			break;

		//
		// Convert to screen space
		//
		physx::PxU32 lineColor = 0xFFAA3300;

		bool willBeOnScreen = true;
		glm::vec3 pointsOnScreen[] = {
			MainLoop::getInstance().camera.PositionToClipSpace(heyho[currentIndex].frustumCornersLightSpace[i]),
			MainLoop::getInstance().camera.PositionToClipSpace(heyho[currentIndex].frustumCornersLightSpace[i - 1])
		};
		for (size_t ii = 0; ii < 2; ii++)
		{
			if (pointsOnScreen[ii].z < 0.0f)
			{
				// Short circuit bc it won't be on screen anymore
				willBeOnScreen = false;
				break;
			}

			pointsOnScreen[ii] /= pointsOnScreen[ii].z;
			pointsOnScreen[ii].x = ImGui::GetWindowPos().x + pointsOnScreen[ii].x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
			pointsOnScreen[ii].y = ImGui::GetWindowPos().y - pointsOnScreen[ii].y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;
		}

		if (!willBeOnScreen)
			continue;

		ImVec2 point1(pointsOnScreen[0].x, pointsOnScreen[0].y);
		ImVec2 point2(pointsOnScreen[1].x, pointsOnScreen[1].y);
		ImGui::GetBackgroundDrawList()->AddLine(point1, point2, lineColor, 1.0f);
	}

	for (size_t i = 1; i < heyho[currentIndex].frustumCornersViewSpace.size(); i += 2)
	{
		if (i > 24)		// Only render the first frustum
			break;

		//
		// Convert to screen space
		//
		physx::PxU32 lineColor = 0xFFAACC00;

		bool willBeOnScreen = true;
		glm::vec3 pointsOnScreen[] = {
			MainLoop::getInstance().camera.PositionToClipSpace(heyho[currentIndex].frustumCornersViewSpace[i]),
			MainLoop::getInstance().camera.PositionToClipSpace(heyho[currentIndex].frustumCornersViewSpace[i - 1])
		};
		for (size_t ii = 0; ii < 2; ii++)
		{
			if (pointsOnScreen[ii].z < 0.0f)
			{
				// Short circuit bc it won't be on screen anymore
				willBeOnScreen = false;
				break;
			}

			pointsOnScreen[ii] /= pointsOnScreen[ii].z;
			pointsOnScreen[ii].x = ImGui::GetWindowPos().x + pointsOnScreen[ii].x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
			pointsOnScreen[ii].y = ImGui::GetWindowPos().y - pointsOnScreen[ii].y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;
		}

		if (!willBeOnScreen)
			continue;

		ImVec2 point1(pointsOnScreen[0].x, pointsOnScreen[0].y);
		ImVec2 point2(pointsOnScreen[1].x, pointsOnScreen[1].y);
		ImGui::GetBackgroundDrawList()->AddLine(point1, point2, lineColor, 1.0f);
	}

	for (size_t i = 1; i < heyho[currentIndex].frustumCornersJojoSpace.size(); i += 2)
	{
		if (i > 24)		// Only render the first frustum
			break;

		//
		// Convert to screen space
		//
		physx::PxU32 lineColor = 0xFF22FF00;

		bool willBeOnScreen = true;
		glm::vec3 pointsOnScreen[] = {
			MainLoop::getInstance().camera.PositionToClipSpace(heyho[currentIndex].frustumCornersJojoSpace[i]),
			MainLoop::getInstance().camera.PositionToClipSpace(heyho[currentIndex].frustumCornersJojoSpace[i - 1])
		};
		for (size_t ii = 0; ii < 2; ii++)
		{
			if (pointsOnScreen[ii].z < 0.0f)
			{
				// Short circuit bc it won't be on screen anymore
				willBeOnScreen = false;
				break;
			}

			pointsOnScreen[ii] /= pointsOnScreen[ii].z;
			pointsOnScreen[ii].x = ImGui::GetWindowPos().x + pointsOnScreen[ii].x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
			pointsOnScreen[ii].y = ImGui::GetWindowPos().y - pointsOnScreen[ii].y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;
		}

		if (!willBeOnScreen)
			continue;

		ImVec2 point1(pointsOnScreen[0].x, pointsOnScreen[0].y);
		ImVec2 point2(pointsOnScreen[1].x, pointsOnScreen[1].y);
		ImGui::GetBackgroundDrawList()->AddLine(point1, point2, lineColor, 1.0f);
	}

	for (size_t i = 0; i < heyho[currentIndex].lightSpaceCenters.size(); i++)
		PhysicsUtils::imguiRenderSphereCollider(glm::translate(glm::mat4(1.0f), heyho[currentIndex].lightSpaceCenters[i]), 0.1f);
}
#endif
