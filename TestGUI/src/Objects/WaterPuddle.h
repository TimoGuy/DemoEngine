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

class WaterPuddle : public BaseObject, public physx::PxSimulationEventCallback
{
public:
	static const std::string TYPE_NAME;

	WaterPuddle();
	~WaterPuddle();

	WaterPuddleImgui* imguiComponent;
	WaterPuddleRender* renderComponent;

	virtual WaterPuddleImgui* getImguiComponent() override { return imguiComponent; }
	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return nullptr; }
	virtual WaterPuddleRender* getRenderComponent() override { return renderComponent; }

	Bounds* bounds;

private:
	physx::PxRigidDynamic* body;
	physx::PxShape* triggerShape;

	void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count);

	// Unused, but need to define
	void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) {}
	void onWake(physx::PxActor** actors, physx::PxU32 count) {}
	void onSleep(physx::PxActor** actors, physx::PxU32 count) {}
	void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) {}
	void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) {}
};
