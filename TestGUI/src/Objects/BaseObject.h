#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <string>
#include <vector>
#include "../Utils/json.hpp"


class BaseObject
{
public:
	BaseObject();
	virtual ~BaseObject() = 0;		// NOTE: no compilation error occurs if the destructor isn't defined dang nabbit

	virtual void loadPropertiesFromJson(nlohmann::json& object);
	virtual nlohmann::json savePropertiesToJson();

	glm::mat4 transform;
	std::string guid;
};

//
// @Cleanup: Random util struct of an AABB that's useful for object selection for imgui and frustum culling
//
struct Bounds
{
	glm::vec3 center;		// NOTE: multiply the model matrix with this to get the world space
	glm::vec3 extents;		// NOTE: this is half the size of the aabb box
};

//
// This indicates that imgui stuff will be drawn from this.
//
class ImGuiComponent
{
public:
	BaseObject* baseObject;
	Bounds* bounds;
	std::string name;

	ImGuiComponent(BaseObject* baseObject, Bounds* bounds, std::string name);
	virtual ~ImGuiComponent();
	virtual void propertyPanelImGui() {}
	virtual void renderImGui();

	virtual void loadPropertiesFromJson(nlohmann::json& object);
	virtual nlohmann::json savePropertiesToJson();

private:
	bool clickPressedPrevious;
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
	bool castsShadows;
	float shadowFarPlane;
	GLuint shadowMapTexture;

	LightComponent(BaseObject* baseObject, bool castsShadows = false);
	virtual ~LightComponent();
	virtual Light& getLight() = 0;			// TODO: Refactor this so that instead of a getLight function it is actually just a light object;
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
	Bounds* bounds = nullptr;

	PhysicsComponent(BaseObject* baseObject, Bounds* bounds);
	virtual ~PhysicsComponent();
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
	Bounds* bounds = nullptr;

	RenderComponent(BaseObject* baseObject, Bounds* bounds);
	virtual ~RenderComponent();

	virtual void preRenderUpdate() = 0;
	virtual void render(unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture) = 0;
	virtual void renderShadow(GLuint programId) = 0;
};
