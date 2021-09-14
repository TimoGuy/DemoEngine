#pragma once

#include "../../Objects/BaseObject.h"
#include "Light.h"
#include "../RenderEngine.camera/Camera.h"


class DirectionalLight : public LightComponent, public ImGuiComponent
{
public:
	DirectionalLight(glm::vec3 eulerAngles);
	~DirectionalLight();

	void setLookDirection(glm::quat rotation);

	Light& getLight() { return light; }
	void propertyPanelImGui();
	void renderImGui();
	void cloneMe();

	glm::mat4 transform;

private:
	Light light;
	unsigned int lightGizmoTextureId;
};
