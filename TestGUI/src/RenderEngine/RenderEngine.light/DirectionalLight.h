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

	Light& getLight() { return light; }

	std::vector<float_t> shadowCascadeLevels;
private:
	Light light;

	//
	// If casting shadows
	//
	void renderPassShadowMap();				// NOTE: here, we'll be using cascaded shadow maps
	void createCSMBuffers();
	std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);
	glm::mat4 getLightSpaceMatrix(const float nearPlane, const float farPlane);
	std::vector<glm::mat4> getLightSpaceMatrices();

	GLuint lightFBO, matricesUBO, cascadedShaderProgram;

	void refreshResources();
};

class DirectionalLight : public BaseObject
{
public:
	DirectionalLight(bool castsShadows);
	~DirectionalLight();

	bool streamTokensForLoading(const std::vector<std::string>& tokens);
	void setLookDirection(glm::quat rotation);

	ImGuiComponent* imguiComponent;
	LightComponent* lightComponent;

	Bounds* bounds;
};
