#pragma once

#include "../../Objects/BaseObject.h"
#include "Light.h"
#include "../RenderEngine.camera/Camera.h"


class DirectionalLight : public LightObject, public ImGuiObject
{
public:
	DirectionalLight(glm::vec3 eulerAngles);

	void setLookDirection(glm::vec3 eulerAngles);

	Light& getLight() { return light; }
	void renderImGui(Camera& camera);

private:
	Light light;
	unsigned int lightGizmoTextureId;
};