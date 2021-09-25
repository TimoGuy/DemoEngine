#pragma once

#include "../../Objects/BaseObject.h"
#include "Light.h"
#include "../RenderEngine.camera/Camera.h"

#include <vector>

		// TODO: pull out this object and place in the objects folder

class DirectionalLightImGui : public ImGuiComponent
{
public:
	DirectionalLightImGui(BaseObject* bo, Bounds* bounds);

	void propertyPanelImGui();
	void renderImGui();
	void cloneMe();

private:
	unsigned int lightGizmoTextureId;

	void refreshResources();
};

class DirectionalLightLight : public LightComponent
{
public:
	DirectionalLightLight(BaseObject* bo, bool castsShadows);
	~DirectionalLightLight();

	Light& getLight() { return light; }

	void createCSMBuffers();

	std::vector<float_t> shadowCascadeLevels;
private:
	Light light;
	bool shadowMapsCreated = false;

	//
	// If casting shadows
	//
	void renderPassShadowMap();				// NOTE: here, we'll be using cascaded shadow maps
	std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);
	glm::mat4 getLightSpaceMatrix(const float nearPlane, const float farPlane);
	std::vector<glm::mat4> getLightSpaceMatrices();

	GLuint lightFBO, matricesUBO, cascadedShaderProgram;

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

	ImGuiComponent* imguiComponent;
	LightComponent* lightComponent;

	Bounds* bounds;
};
