#pragma once

#include "BaseObject.h"
#include "../render_engine/model/animation/Animator.h"


class WaterPuddleRender : public RenderComponent
{
public:
	WaterPuddleRender(BaseObject* bo);

	void preRenderUpdate();
	void render();
	void renderShadow(GLuint programId);

private:
	void refreshResources();

	Model* model;
	Animator animator;
	std::map<std::string, Material*> materials;
};

class WaterPuddle : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	WaterPuddle();
	~WaterPuddle();

	void loadPropertiesFromJson(nlohmann::json& object);
	nlohmann::json savePropertiesToJson();

	PhysicsComponent* physicsComponent;
	WaterPuddleRender* renderComponent;

	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return physicsComponent; }
	virtual WaterPuddleRender* getRenderComponent() override { return renderComponent; }

	void onTrigger(const physx::PxTriggerPair& pair);

	void collectWaterPuddle();
	bool isWaterPuddleCollected;

	inline bool isBeingTriggeredByPlayer() { return beingTriggeredByPlayer; }

#ifdef _DEBUG
	void propertyPanelImGui();
	void renderImGui();
#endif

private:
	bool beingTriggeredByPlayer = false;
};
