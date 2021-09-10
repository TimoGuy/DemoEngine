#pragma once

#include <glm/glm.hpp>


//
// This indicates that imgui stuff will be drawn from this.
//
class ImGuiObject
{
public:
	char* name;
	glm::mat4& transform;

	ImGuiObject(char* name, glm::mat4& transform) : name(name), transform(transform) {}
	virtual void propertyPanelImGui() {}
	virtual void renderImGui() = 0;
};


//
// This indicates that a camera is extractable
// from this object. Will be placed in the cameraObjects
// queue (ideally, you're gonna have to do that manually, bud).
//
class Camera;
class CameraObject
{
public:
	glm::mat4& transform;

	CameraObject(glm::mat4& transform) : transform(transform) {}
	virtual Camera& getCamera() = 0;
};


//
// This indicates that a light is extractable
// from this object. Will be placed in the lightobjects queue.
//
struct Light;
class LightObject
{
public:
	glm::mat4& transform;

	LightObject(glm::mat4& transform) : transform(transform) {}
	virtual Light& getLight() = 0;
};


//
// This indicates that the object wants physics to be called.
// Will (hopefully) be added into the physicsobjects queue.
//
class PhysicsObject
{
public:
	glm::mat4& transform;

	PhysicsObject(glm::mat4& transform) : transform(transform) {}
	virtual void physicsUpdate(float deltaTime) = 0;
};


//
// This indicates that the object wants to be rendered.
// Will be added into the renderobjects queue.
//
class RenderObject
{
public:
	glm::mat4& transform;

	RenderObject(glm::mat4& transform) : transform(transform) {}
	virtual void render(bool shadowPass, unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture, unsigned int shadowMapTexture) = 0;			// TODO: figure out what they need and implement it!
};
