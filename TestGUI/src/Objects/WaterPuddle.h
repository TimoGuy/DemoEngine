#pragma once

#include "BaseObject.h"


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
	void render(unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture);
	void renderShadow(GLuint programId);
};

class WaterPuddle : public BaseObject, public TriggerComponent
{
public:
	static const std::string TYPE_NAME;

	WaterPuddle();
	~WaterPuddle();

	void loadPropertiesFromJson(nlohmann::json& object);
	nlohmann::json savePropertiesToJson();

	WaterPuddleImgui* imguiComponent;
	WaterPuddleRender* renderComponent;

	virtual WaterPuddleImgui* getImguiComponent() override { return imguiComponent; }
	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return nullptr; }
	virtual WaterPuddleRender* getRenderComponent() override { return renderComponent; }

	void notifyNewTransform(glm::mat4 newTransform);

	Bounds* bounds;

private:
	physx::PxRigidDynamic* body;
	physx::PxShape* triggerShape;

	physx::PxActor* getActor();
	void onTrigger(const physx::PxTriggerPair& pair);
};
