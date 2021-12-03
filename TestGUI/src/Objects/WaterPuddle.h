#pragma once

#include "BaseObject.h"
#include "../RenderEngine/RenderEngine.model/RenderEngine.model.animation/Animator.h"


class WaterPuddleRender : public RenderComponent
{
public:
	WaterPuddleRender(BaseObject* bo, RenderAABB* bounds);

	void preRenderUpdate();
	void render();
	void renderShadow(GLuint programId);

	inline glm::mat4& getOffsetModelMatrix() { return offsetModelMatrix; }

private:
	void refreshResources();

	Model* model;
	Animator animator;
	std::map<std::string, Material*> materials;

	glm::mat4 offsetModelMatrix;
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

	RenderAABB* bounds;

	inline bool isBeingTriggeredByPlayer() { return beingTriggeredByPlayer; }

#ifdef _DEBUG
	void propertyPanelImGui();
	void renderImGui();
#endif

private:
	bool beingTriggeredByPlayer = false;
};
