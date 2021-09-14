#pragma once

#include <glm/glm.hpp>


//
// This indicates that imgui stuff will be drawn from this.
//
class ImGuiComponent
{
public:
	char* name;
	glm::mat4& transform;

	ImGuiComponent(char* name, glm::mat4& transform);
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
	glm::mat4& transform;

	CameraComponent(glm::mat4& transform);
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
	glm::mat4& transform;

	LightComponent(glm::mat4& transform);
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
	glm::mat4& transform;

	PhysicsComponent(glm::mat4& transform);
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
	glm::mat4& transform;

	RenderComponent(glm::mat4& transform);
	~RenderComponent();
	virtual void render(bool shadowPass, unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture, unsigned int shadowMapTexture) = 0;			// TODO: figure out what they need and implement it!
};
