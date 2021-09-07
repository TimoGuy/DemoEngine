#pragma once


//
// This indicates that imgui stuff will be drawn from this.
//
class Camera;
class ImGuiObject
{
public:
	virtual void renderImGui(Camera& camera) = 0;
};


//
// This indicates that a camera is extractable
// from this object. Will be placed in the cameraObjects
// queue (ideally, you're gonna have to do that manually, bud).
//
class CameraObject
{
public:
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
	virtual Light& getLight() = 0;
};


//
// This indicates that the object wants physics to be called.
// Will (hopefully) be added into the physicsobjects queue.
//
class PhysicsObject
{
public:
	virtual void physicsUpdate(float deltaTime) = 0;
};


//
// This indicates that the object wants to be rendered.
// Will be added into the renderobjects queue.
//
class RenderObject
{
public:
	virtual void render(bool shadowPass, Camera& camera, unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture, unsigned int shadowMapTexture) = 0;			// TODO: figure out what they need and implement it!
};
