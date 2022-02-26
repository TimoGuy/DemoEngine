#pragma once

#include "BaseObject.h"
#include "../render_engine/camera/Camera.h"

#include <vector>
class Shader;

class DirectionalLightLight : public LightComponent
{
public:
	DirectionalLightLight(BaseObject* bo, bool castsShadows);
	~DirectionalLightLight();

	void refreshRenderBuffers();

	std::vector<float_t> shadowCascadeLevels;
	std::vector<float_t> shadowCascadeTexelSizes;

	// Light intensity based off angle
	// @DEBUG: putting into public, but really should be in private bc imgui yo
	float maxColorIntensity;
	float colorIntensityMaxAtY;
private:

	// CSM shadows
	bool shadowMapsCreated = false;

	void createCSMBuffers();
	void destroyCSMBuffers();

	//
	// If casting shadows
	//
	void renderPassShadowMap();				// NOTE: here, we'll be using cascaded shadow maps
	std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);
	glm::mat4 getLightSpaceMatrix(const float nearPlane, const float farPlane);
	std::vector<glm::mat4> getLightSpaceMatrices();

	GLuint lightFBO, matricesUBO;
	Shader* csmShader;

	void refreshResources();
};

class DirectionalLight : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	DirectionalLight(bool castsShadows);
	~DirectionalLight();

	void setLookDirection(glm::quat rotation);

	void loadPropertiesFromJson(nlohmann::json& object);
	nlohmann::json savePropertiesToJson();

	LightComponent* lightComponent;

	LightComponent* getLightComponent() { return lightComponent; }
	PhysicsComponent* getPhysicsComponent() { return nullptr; }
	RenderComponent* getRenderComponent() { return nullptr; }

	void preRenderUpdate();

#ifdef _DEVELOP
	void imguiPropertyPanel();
	void imguiRender();
#endif

private:
	unsigned int lightGizmoTextureId;

	glm::vec3 hirumaColor, hiokureColor;

	void refreshResources();
};
