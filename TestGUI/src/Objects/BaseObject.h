#pragma once

#include <glm/glm.hpp>


//
// This contains all the basic transform information
//
class BaseObject
{
public:
	glm::mat4 transform = glm::mat4(1.0f);
};

//
// This indicates that imgui stuff will be drawn from this.
//
class ImGuiObject : public BaseObject
{
public:
	virtual void propertyPanelImGui() {}
	virtual void renderImGui() = 0;
};


//
// This indicates that a camera is extractable
// from this object. Will be placed in the cameraObjects
// queue (ideally, you're gonna have to do that manually, bud).
//
class Camera;
class CameraObject : public BaseObject
{
public:
	virtual Camera& getCamera() = 0;
};


//
// This indicates that a light is extractable
// from this object. Will be placed in the lightobjects queue.
//
struct Light;
class LightObject : public BaseObject
{
public:
	virtual Light& getLight() = 0;
};


//
// This indicates that the object wants physics to be called.
// Will (hopefully) be added into the physicsobjects queue.
//
class PhysicsObject : public BaseObject
{
public:
	virtual void physicsUpdate(float deltaTime) = 0;
};


//
// This indicates that the object wants to be rendered.
// Will be added into the renderobjects queue.
//
class RenderObject : public BaseObject
{
public:
	virtual void render(bool shadowPass, unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture, unsigned int shadowMapTexture) = 0;			// TODO: figure out what they need and implement it!
};
