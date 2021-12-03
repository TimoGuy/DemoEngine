#pragma once

#include "BaseObject.h"
#include "../RenderEngine/RenderEngine.camera/Camera.h"


class PointLightImGui : public ImGuiComponent
{
public:
	PointLightImGui(BaseObject* bo, RenderAABB* bounds);

	void propertyPanelImGui();
	void renderImGui();

private:
	unsigned int lightGizmoTextureId;

	void refreshResources();
};

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

	ImGuiComponent* imguiComponent;
	LightComponent* lightComponent;

	ImGuiComponent* getImguiComponent() { return imguiComponent; }
	LightComponent* getLightComponent() { return lightComponent; }
	PhysicsComponent* getPhysicsComponent() { return nullptr; }
	RenderComponent* getRenderComponent() { return nullptr; }

	RenderAABB* bounds;
};
