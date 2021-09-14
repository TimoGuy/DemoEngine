#pragma once

#include "../../Objects/BaseObject.h"
#include "Light.h"
#include "../RenderEngine.camera/Camera.h"


class DirectionalLightImGui : public ImGuiComponent
{
public:
	DirectionalLightImGui(BaseObject* bo);

	void propertyPanelImGui();
	void renderImGui();
	void cloneMe();

private:
	unsigned int lightGizmoTextureId;
};

class DirectionalLightLight : public LightComponent
{
public:
	DirectionalLightLight(BaseObject* bo);

	Light& getLight() { return light; }

private:
	Light light;
};

class DirectionalLight : public BaseObject
{
public:
	DirectionalLight(glm::vec3 eulerAngles);
	~DirectionalLight();

	void setLookDirection(glm::quat rotation);

	ImGuiComponent* imguiComponent;
	LightComponent* lightComponent;
};
