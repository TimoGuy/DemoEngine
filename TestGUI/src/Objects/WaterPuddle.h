#pragma once

#include "BaseObject.h"


class WaterPuddle : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	ImGuiComponent* imguiComponent;
	PhysicsComponent* physicsComponent;		// NOTE: maybe not here, bc it's just a trigger for the puddle???
	RenderComponent* renderComponent;

	ImGuiComponent* getImguiComponent() { return imguiComponent; }
	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return physicsComponent; }
	RenderComponent* getRenderComponent() { return renderComponent; }

	Bounds* bounds;
};


class WaterPuddleImgui : public ImGuiComponent
{
public:
	WaterPuddleImgui(BaseObject* bo, Bounds* bounds) : ImGuiComponent(bo, bounds, "Water Puddle") {}

	void propertyPanelImGui();
	void renderImGui();
};


class WaterPuddlePhysics : public PhysicsComponent
{
	// NOTE: this may not be needed to do the physics, since it'd just be one spherical trigger
public:
	void physicsUpdate();
	physx::PxTransform getGlobalPose();
	void propagateNewTransform(const glm::mat4& newTransform);
};


class WaterPuddleRender : public RenderComponent
{
public:
	WaterPuddleRender(BaseObject* bo, Bounds* bounds) : RenderComponent(bo, bounds) {}

	void preRenderUpdate();
	void render(unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture);
	void renderShadow(GLuint programId);
};
