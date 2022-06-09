#pragma once

#include "BaseObject.h"
#include "../render_engine/model/animation/Animator.h"
#include "../render_engine/model/animation/AnimatorStateMachine.h"


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
	int numWaterServings;

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
	AnimatorStateMachine animatorStateMachine;
	std::map<std::string, Material*> materials;
	TextRenderer waterServingsText;
};
