#pragma once


//
// The different types of objects will extend
// from this, that way there can be a common type
// inside of the queues of MainLoop
//
class BaseObject{};


//
// This indicates that a camera is extractable
// from this object. Will be placed in the cameraObjects
// queue (ideally, you're gonna have to do that manually, bud).
//
class Camera;
class CameraObject : BaseObject
{
public:
	virtual Camera& getCamera() = 0;
};


//
// This indicates that a light is extractable
// from this object. Will be placed in the lightobjects queue.
//
class Light;
class LightObject : BaseObject
{
public:
	virtual Light& getLight() = 0;
};


//
// This indicates that the object wants physics to be called.
// Will (hopefully) be added into the physicsobjects queue.
//
class PhysicsObject : BaseObject
{
public:
	virtual void physicsUpdate(float deltaTime) = 0;
};


//
// This indicates that the object wants to be rendered.
// Will be added into the renderobjects queue.
//
class RenderObject : BaseObject
{
public:
	virtual void render() = 0;			// TODO: figure out what they need and implement it!
};
