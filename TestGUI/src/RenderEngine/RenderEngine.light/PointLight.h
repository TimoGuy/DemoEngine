#pragma once

#include "../../Objects/BaseObject.h"
#include "Light.h"
#include "../RenderEngine.camera/Camera.h"


class PointLightImGui : public ImGuiComponent
{
public:
	PointLightImGui(BaseObject* bo, Bounds* bounds);

	void propertyPanelImGui();
	void renderImGui();
	void cloneMe();

private:
	unsigned int lightGizmoTextureId;

	void refreshResources();
};

class PointLightLight : public LightComponent
{
public:
	PointLightLight(BaseObject* bo, bool castsShadows);
	~PointLightLight();

	Light& getLight() { return light; }

	void refreshShadowBuffers();

	float nearPlane = 1.0f, farPlane = 25.0f;

	GLuint pointLightShaderProgram, lightFBO;
private:
	Light light;
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

	Bounds* bounds;
};
