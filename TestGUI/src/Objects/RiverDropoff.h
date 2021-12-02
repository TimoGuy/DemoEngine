#pragma once

#include "BaseObject.h"


class Material;
class Model;

class RiverDropoffImgui : public ImGuiComponent
{
public:
	RiverDropoffImgui(BaseObject* bo, RenderAABB* bounds) : ImGuiComponent(bo, bounds, "River Dropoff Area") {}

	void propertyPanelImGui();
	void renderImGui();
};


class RiverDropoffRender : public RenderComponent
{
public:
	RiverDropoffRender(BaseObject* bo, RenderAABB* bounds);

	void preRenderUpdate();
	void render();
	void renderShadow(GLuint programId);

private:
	void refreshResources();
	Model* model;
	std::map<std::string, Material*> materials;
};

class RiverDropoff : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	RiverDropoff();
	~RiverDropoff();

	void loadPropertiesFromJson(nlohmann::json& object);
	nlohmann::json savePropertiesToJson();

	RiverDropoffImgui* imguiComponent;
	PhysicsComponent* physicsComponent;
	RiverDropoffRender* renderComponent;

	virtual RiverDropoffImgui* getImguiComponent() override { return imguiComponent; }
	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return physicsComponent; }
	virtual RiverDropoffRender* getRenderComponent() override { return renderComponent; }

	void onTrigger(const physx::PxTriggerPair& pair);

	void dropoffWater();

	RenderAABB* bounds;

	inline bool isBeingTriggeredByPlayer() { return beingTriggeredByPlayer; }

private:
	bool beingTriggeredByPlayer = false;
};
