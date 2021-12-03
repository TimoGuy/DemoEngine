#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <PxPhysicsAPI.h>
#include "../Utils/json.hpp"


typedef unsigned int GLuint;

class LightComponent;
class PhysicsComponent;
class RenderComponent;

struct PhysicsTransformState
{
	glm::mat4 previousTransform;
	glm::mat4 currentTransform;

	void updateTransform(glm::mat4 newTransform);
	glm::mat4 getInterpolatedTransform(float alpha);
};

class BaseObject
{
public:
	BaseObject();
	virtual ~BaseObject() = 0;		// NOTE: no compilation error occurs if the destructor isn't defined dang nabbit

	virtual void loadPropertiesFromJson(nlohmann::json& object);
	virtual nlohmann::json savePropertiesToJson();

	virtual LightComponent* getLightComponent() = 0;
	virtual PhysicsComponent* getPhysicsComponent() = 0;
	virtual RenderComponent* getRenderComponent() = 0;

#ifdef _DEBUG
	virtual void propertyPanelImGui() {}
	virtual void renderImGui() {}
#endif

	glm::mat4& getTransform();
	glm::mat4 getTransformWithoutScale();				// NOTE: this is not a getter; it computes the transform without the scale
	void setTransform(glm::mat4 newTransform);

	std::string guid;
	std::string name;

	//
	// Callback functions (optional)
	//
	virtual void physicsUpdate() {}
	virtual void onTrigger(const physx::PxTriggerPair& pair) {}

	//
	// INTERNAL FUNCTIONS (for physics)
	//
	void INTERNALsubmitPhysicsCalculation(glm::mat4 newTransform);
	void INTERNALfetchInterpolatedPhysicsTransform(float alpha);

private:
	glm::mat4 transform;

	PhysicsTransformState physicsTransformState;		// INTERNAL for physics
};

//
// @Cleanup: Random util struct of an AABB that's useful for object selection for imgui and frustum culling
//
struct RenderAABB
{
	glm::vec3 center;		// NOTE: multiply the baseObject->transform with this to get the world space
	glm::vec3 extents;		// NOTE: this is half the size of the aabb box
};


//
// This indicates that a light is extractable
// from this object. Will be placed in the lightobjects queue.
//
enum LightType { DIRECTIONAL, POINT, SPOT };
class LightComponent
{
public:
	// LIGHT STRUCT
	LightType lightType;

	glm::vec3 facingDirection;
	glm::vec3 color;
	float colorIntensity;

	// REGULAR STUFF
	BaseObject* baseObject;
	bool castsShadows;						// NOTE: when this is false, for some reason when you resize the window (or if it's off on startup), then the whole GL app just... crashes and goes into accum buffer mode and has a black screen except for imgui stuff
	float shadowFarPlane;
	GLuint shadowMapTexture;

	LightComponent(BaseObject* baseObject, bool castsShadows = false);
	virtual ~LightComponent();
	virtual void renderPassShadowMap();

	virtual void loadPropertiesFromJson(nlohmann::json& object);
	virtual nlohmann::json savePropertiesToJson();
};


//
// This indicates that the object wants physics to be called.
// Will (hopefully) be added into the physicsobjects queue.
//
class PhysicsComponent
{
public:
	BaseObject* baseObject;

	PhysicsComponent(BaseObject* baseObject);
	virtual ~PhysicsComponent();
	virtual void physicsUpdate() = 0;
	virtual physx::PxTransform getGlobalPose() = 0;

	virtual void propagateNewTransform(const glm::mat4& newTransform) = 0;

	physx::PxRigidActor* getActor();
	void INTERNALonTrigger(const physx::PxTriggerPair& pair);

protected:
	physx::PxRigidActor* body;
};


//
// This indicates that the object wants to be rendered.
// Will be added into the renderobjects queue.
//
class RenderComponent
{
public:
	BaseObject* baseObject;
	RenderAABB* bounds = nullptr;

	RenderComponent(BaseObject* baseObject, RenderAABB* bounds);
	virtual ~RenderComponent();

	virtual void preRenderUpdate() = 0;
	virtual void render() = 0;
	virtual void renderShadow(GLuint programId) = 0;

	// NOTE: this transform could be messed up due to 3d model offset placement, or other factors, so with anything that isn't related to rendering, like frustum culling, for example, pls do not use this value.
	const glm::mat4& getRenderTransform();

protected:
	glm::mat4 renderTransform;
};
