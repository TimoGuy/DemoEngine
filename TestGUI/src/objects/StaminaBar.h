#pragma once

#include "BaseObject.h"


class Material;
class Model;

class StaminaBar : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	StaminaBar();
	~StaminaBar();

	void refreshResources();
	void preRenderUpdate();

	void loadPropertiesFromJson(nlohmann::json& object);
	nlohmann::json savePropertiesToJson();

	RenderComponent* renderComponent;

	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return nullptr; }
	RenderComponent* getRenderComponent() { return renderComponent; }

#ifdef _DEVELOP
	void imguiPropertyPanel();
	void imguiRender();
#endif

private:
	Model* model;
	std::map<std::string, Material*> materials;
};
