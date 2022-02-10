#pragma once

#include "BaseObject.h"
#include "../render_engine/camera/Camera.h"

class Shader;


class PointLightLight : public LightComponent
{
public:
	PointLightLight(BaseObject* bo, bool castsShadows);
	~PointLightLight();

	void refreshShadowBuffers();

	float nearPlane = 1.0f, farPlane = 25.0f;

	GLuint lightFBO;
	Shader* pointLightShadowShader;
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

#ifdef _DEVELOP
	static bool showLightVolumes;
	void imguiPropertyPanel();
	void imguiRender();
#endif

private:
	unsigned int lightGizmoTextureId;

	void refreshResources();
};
