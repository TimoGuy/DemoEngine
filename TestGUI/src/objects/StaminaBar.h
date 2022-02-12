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

	PhysicsComponent* physicsComponent;
	RenderComponent* renderComponent;

	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return physicsComponent; }
	RenderComponent* getRenderComponent() { return renderComponent; }

	void onTrigger(const physx::PxTriggerPair& pair);

	void dropoffWater();

	inline bool isBeingTriggeredByPlayer() { return beingTriggeredByPlayer; }

#ifdef _DEVELOP
	void imguiPropertyPanel();
	void imguiRender();
#endif

private:
	bool beingTriggeredByPlayer = false;

	//
	// OLD RIVERDROPOFFRENDER
	//
	Model* model;
	std::map<std::string, Material*> materials;
};
