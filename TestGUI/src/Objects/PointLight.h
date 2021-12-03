#pragma once

#include "BaseObject.h"
#include "../RenderEngine/RenderEngine.camera/Camera.h"


class PointLightLight : public LightComponent
{
public:
	PointLightLight(BaseObject* bo, bool castsShadows);
	~PointLightLight();

	void refreshShadowBuffers();

	float nearPlane = 1.0f, farPlane = 25.0f;

	GLuint pointLightShaderProgram, lightFBO;
private:
	bool shadowMapsCreated = false;

	std::vector<glm::mat4> shadowTransforms;

	void createShadowBuffers();
	void destroyShadowBuffers();

	void configureShaderAndMatrices();
	void renderPassShadowMap();

	void refreshResources();
};

class PointLight : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	PointLight(bool castsShadows);
	~PointLight();

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
