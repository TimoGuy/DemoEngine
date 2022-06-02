#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <PxPhysicsAPI.h>
#include "../utils/json.hpp"


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

	//
	// Required functions
	//
	virtual void refreshResources() = 0;		// NOTE: this inserts in models, materials, etc. into the various components. This doesn't recreate the physics shapes or anything.

	virtual void loadPropertiesFromJson(nlohmann::json& object) = 0;
	virtual nlohmann::json savePropertiesToJson() = 0;

	virtual LightComponent* getLightComponent() = 0;
	virtual PhysicsComponent* getPhysicsComponent() = 0;
	virtual RenderComponent* getRenderComponent() = 0;

#ifdef _DEVELOP
	virtual void imguiPropertyPanel() {}
	virtual void imguiRender() {}
#endif

	glm::mat4& getTransform();
	glm::mat4 getTransformWithoutScale();				// NOTE: this is not a getter; it computes the transform without the scale
	void setTransform(glm::mat4 newTransform);

	std::string guid;
	std::string name;

	//
	// Callback functions (optional)
	//
	virtual void preRenderUpdate() {};
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

#ifdef _DEVELOP
// This is for creating a rendercomponent that you need to connect to 0,0,0
class DummyBaseObject : public BaseObject
{
public:
	DummyBaseObject() { setTransform(glm::mat4(1.0f)); }
	~DummyBaseObject() {}

	void loadPropertiesFromJson(nlohmann::json& object) {}
	nlohmann::json savePropertiesToJson() { nlohmann::json j; return j; }

	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return nullptr; }
	RenderComponent* getRenderComponent() { return nullptr; }

	void refreshResources() {}
};
#endif


//
// This indicates that a light is extractable
// from this object. Will be placed in the lightobjects queue.
//
enum class LightType { DIRECTIONAL, POINT, SPOT };
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
class Model;
class Animator;
struct ModelWithMetadata
{
	Model* model;
	bool renderModelInShadow;
	Animator* modelAnimator;
};

enum class TextAlignment
{
	LEFT,
	RIGHT,
	CENTER,
	TOP = LEFT,		// Copies
	BOTTOM = RIGHT	// Copies
};

struct TextRenderer
{
	std::string text;
	glm::mat4 modelMatrix;
	glm::vec3 color;
	TextAlignment horizontalAlign;
	TextAlignment verticalAlign;
};

class Shader;
struct ViewFrustum;
class RenderComponent final
{
public:
	BaseObject* baseObject;

	RenderComponent(BaseObject* baseObject);
	~RenderComponent();

	// @TODO: @GIANT: You need to add a function to the main game loop to update the animator. This will finally use the modelAniamtor field...

	void addModelToRender(const ModelWithMetadata& modelWithMetadata);
	void clearAllModels();

	void addTextToRender(TextRenderer* textRenderer);

	void render(const ViewFrustum* viewFrustum, Shader* zPassShader);
	void renderShadow(Shader* shader);

#ifdef _DEVELOP
	std::vector<std::string> getMaterialNameList();
	void TEMPrenderImguiModelBounds();
#endif

private:
	std::vector<ModelWithMetadata> modelsWithMetadata;
	std::vector<TextRenderer*> textRenderers;

#ifdef _DEVELOP
	void refreshResources();
#endif
};
