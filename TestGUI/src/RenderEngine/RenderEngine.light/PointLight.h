#pragma once

#include "../../Objects/BaseObject.h"
#include "Light.h"
#include "../RenderEngine.camera/Camera.h"


class PointLightImGui : public ImGuiComponent
{
public:
	PointLightImGui(BaseObject* bo);

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
	PointLightLight(BaseObject* bo);

	Light& getLight() { return light; }

private:
	Light light;
};

class PointLight : public BaseObject
{
public:
	PointLight();
	~PointLight();

	ImGuiComponent* imguiComponent;
	LightComponent* lightComponent;
};
