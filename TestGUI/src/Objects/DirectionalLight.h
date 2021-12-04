#pragma once

#include "BaseObject.h"
#include "../render_engine/camera/Camera.h"

#include <vector>


class DirectionalLightLight : public LightComponent
{
public:
	DirectionalLightLight(BaseObject* bo, bool castsShadows);
	~DirectionalLightLight();

	void refreshRenderBuffers();

	std::vector<float_t> shadowCascadeLevels;
private:
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

	LightComponent* lightComponent;

	LightComponent* getLightComponent() { return lightComponent; }
	PhysicsComponent* getPhysicsComponent() { return nullptr; }
	RenderComponent* getRenderComponent() { return nullptr; }

	RenderAABB* bounds;

#ifdef _DEBUG
	void propertyPanelImGui();
	void renderImGui();
#endif

private:
	unsigned int lightGizmoTextureId;

	void refreshResources();
};
