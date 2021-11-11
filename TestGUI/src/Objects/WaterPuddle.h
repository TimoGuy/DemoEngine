#pragma once

#include "BaseObject.h"
#include "../RenderEngine/RenderEngine.model/RenderEngine.model.animation/Animator.h"


class WaterPuddleImgui : public ImGuiComponent
{
public:
	WaterPuddleImgui(BaseObject* bo, Bounds* bounds) : ImGuiComponent(bo, bounds, "Water Puddle") {}

	void propertyPanelImGui();
	void renderImGui();
};


class WaterPuddleRender : public RenderComponent
{
public:
	WaterPuddleRender(BaseObject* bo, Bounds* bounds);

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

	bool importedAnims = false;		// TODO: this should not be here.... it's only here bc of debug mode wanting to hot reload animations. THERE NEEDS TO BE A BETTER WAY!!!
};

class WaterPuddle : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	WaterPuddle();
	~WaterPuddle();

	void loadPropertiesFromJson(nlohmann::json& object);
	nlohmann::json savePropertiesToJson();

	WaterPuddleImgui* imguiComponent;
	PhysicsComponent* physicsComponent;
	WaterPuddleRender* renderComponent;

	virtual WaterPuddleImgui* getImguiComponent() override { return imguiComponent; }
	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return physicsComponent; }
	virtual WaterPuddleRender* getRenderComponent() override { return renderComponent; }

	void onTrigger(const physx::PxTriggerPair& pair);

	void collectWaterPuddle();
	bool isWaterPuddleCollected;

	Bounds* bounds;

	inline bool isBeingTriggeredByPlayer() { return beingTriggeredByPlayer; }

private:
	bool beingTriggeredByPlayer = false;
};
