#pragma once

#include "BaseObject.h"
#include "../render_engine/model/animation/Animator.h"


class WaterPuddle : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	WaterPuddle();
	~WaterPuddle();

	void preRenderUpdate();

	void loadPropertiesFromJson(nlohmann::json& object);
	nlohmann::json savePropertiesToJson();

	PhysicsComponent* physicsComponent;
	RenderComponent* renderComponent;

	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return physicsComponent; }
	RenderComponent* getRenderComponent() { return renderComponent; }

	void refreshResources();

	void onTrigger(const physx::PxTriggerPair& pair);

	void collectWaterPuddle();
	bool containsWater;

	inline bool isBeingTriggeredByPlayer() { return beingTriggeredByPlayer; }

#ifdef _DEVELOP
	void imguiPropertyPanel();
	void imguiRender();
#endif

private:
	bool beingTriggeredByPlayer = false;

	//
	// OLD WATERPUDDLERENDER
	//
	Model* model;
	Animator animator;
	std::map<std::string, Material*> materials;
};
