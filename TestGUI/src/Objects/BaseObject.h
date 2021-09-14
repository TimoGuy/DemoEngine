#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <string>


class BaseObject
{
public:
	virtual ~BaseObject()
	{
		// NOTE: this note is lying, so i commented it out
		//std::cout << "WARNING: BaseObject Destructor not overridden. Undefined behavior may occur" << std::endl;
	}

	glm::mat4 transform;
};


//
// This indicates that imgui stuff will be drawn from this.
//
class ImGuiComponent
{
public:
	BaseObject* baseObject;
	std::string name;
	std::string guid;

	ImGuiComponent(BaseObject* baseObject, std::string name);
	~ImGuiComponent();
	virtual void propertyPanelImGui() {}
	virtual void renderImGui() = 0;
	virtual void cloneMe() = 0;
};


//
// This indicates that a camera is extractable
// from this object. Will be placed in the cameraObjects
// queue (ideally, you're gonna have to do that manually, bud).
//
class Camera;
class CameraComponent
{
public:
	BaseObject* baseObject;

	CameraComponent(BaseObject* baseObject);
	~CameraComponent();
	virtual Camera& getCamera() = 0;
};


//
// This indicates that a light is extractable
// from this object. Will be placed in the lightobjects queue.
//
struct Light;
class LightComponent
{
public:
	BaseObject* baseObject;

	LightComponent(BaseObject* baseObject);
	~LightComponent();
	virtual Light& getLight() = 0;
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
	~PhysicsComponent();
	virtual void physicsUpdate(float deltaTime) = 0;
};


//
// This indicates that the object wants to be rendered.
// Will be added into the renderobjects queue.
//
class RenderComponent
{
public:
	BaseObject* baseObject;

	RenderComponent(BaseObject* baseObject);
	~RenderComponent();
	virtual void render(bool shadowPass, unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture, unsigned int shadowMapTexture) = 0;			// TODO: figure out what they need and implement it!
};
