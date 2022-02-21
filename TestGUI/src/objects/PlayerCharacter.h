#pragma once

#include <PxPhysicsAPI.h>
#include "BaseObject.h"
#include "../render_engine/model/animation/Animator.h"
#include "../render_engine/camera/Camera.h"

typedef unsigned int GLuint;


//
// TODO: place this class inside of somewhere!!!
//
class RopeSimulation
{
public:
	void initializePoints(const std::vector<glm::vec3>& points);
	void setPointPosition(size_t index, float trickleRate, const glm::vec3& position);

	void simulateRope(float gravityMultiplier);

	glm::vec3 getPoint(size_t index) { return points[index]; }

	bool isFirstTime = true;
	bool limitTo45degrees = false;

private:
	std::vector<glm::vec3> points;
	std::vector<glm::vec3> prevPoints;
	std::vector<float> distances;
};


class PlayerPhysics : public PhysicsComponent, public physx::PxUserControllerHitReport, public physx::PxControllerBehaviorCallback
{
public:
	PlayerPhysics(BaseObject* bo);
	~PlayerPhysics();

	void lockVelocity(bool yAlso);

	void physicsUpdate();
	void propagateNewTransform(const glm::mat4& newTransform);
	physx::PxTransform getGlobalPose();

	physx::PxVec3 velocity;

	physx::PxCapsuleController* controller;
	physx::PxVec3 tempUp = physx::PxVec3(0.0f, 1.0f, 0.0f);

	void setIsGrounded(bool flag) { isGrounded = flag; }
	bool getIsGrounded() { return isGrounded; }
	void setIsSliding(bool flag) { isSliding = flag; }
	bool getIsSliding() { return isSliding; }
	bool getStandingOnAngularVelocityY(float& dipstick) { dipstick = standingOnAngularVelocityYRadians; return hasValidStandingOnAngularVelocityY; }

	glm::vec3 getGroundedNormal() { return groundHitNormal; }	// NOTE: this should not be accessed while !isGrounded, bc it isn't updated unless if on ground <45degrees

	// PxUserControllerHitReport methods
	virtual void onShapeHit(const physx::PxControllerShapeHit& hit);
	virtual void onControllerHit(const physx::PxControllersHit& hit);
	virtual void onObstacleHit(const physx::PxControllerObstacleHit& hit);

	virtual physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxShape& shape, const physx::PxActor& actor);
	virtual physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxController&);
	virtual physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxObstacle&);

private:
	bool isGrounded = false;
	bool isSliding = false;
	bool isSlidingCeiling = false;
	bool isSandwiching = false;		// NOTE: this is where you push yourself into a crevice (defined by collision on bottom and top)
	bool hasValidStandingOnAngularVelocityY = false;
	float standingOnAngularVelocityYRadians;
	glm::vec3 groundHitNormal = glm::vec3(0, 1, 0);
	glm::vec3 ceilingHitNormal = glm::vec3(0, -1, 0);
	physx::PxExtendedVec3 offsetFootMovedReconstructed;		// NOTE: this is only valid/updated when the hitnormal.y >= 0, so essentially if it's a bottom-hit
	physx::PxExtendedVec3 offsetHeadMovedReconstructed;		// NOTE: this is only valid/updated when the hitnormal.y < 0, so essentially if it's a top-hit

	// For retaining velocity when leaving the ground
	bool prevIsGrounded = false;
	physx::PxVec3 prevPositionWhileGrounded;
};


struct VirtualCamera;

class PlayerCharacter : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	PlayerCharacter();
	~PlayerCharacter();

	void loadPropertiesFromJson(nlohmann::json& object);
	nlohmann::json savePropertiesToJson();

	PhysicsComponent* physicsComponent;
	RenderComponent* renderComponent;

	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return physicsComponent; }
	RenderComponent* getRenderComponent() { return renderComponent; }

	void preRenderUpdate();

#ifdef _DEVELOP
	void imguiPropertyPanel();
	void imguiRender();
#endif

	//
	// OLD PLAYERRENDER
	//
	Model* model;
	Animator animator;
	std::map<std::string, Material*> materials;

	Model* bottleModel;
	std::map<std::string, Material*> bottleModelMaterials;
	glm::mat4 bottleModelMatrix, bottleHandModelMatrix;

	glm::vec3 cameraPosition;
	float cameraSpeedMultiplier = 1000.0f;		// Originally was 10.0f

	float currentMaxCamDistance = 0;
	float maxCamDistanceHoldTime = 1.0f, maxCamDistanceHoldTimer = 0;
	float maxCamDistanceShiftSpeed = 1.0f;
	glm::vec3 playerCamOffset = glm::vec3(0, 3, -30);
	glm::vec3 playerCamOffsetIndoor = glm::vec3(0, 1.5f, -10.5f);

	glm::vec2 lookingInput = glm::vec2(0, 0);					// [0-360) on x axis (degrees), [-1,1] on y axis
	glm::vec2 lookingSensitivity = glm::vec2(0.2f, 0.001f);		// Sensitivity for how much the amount moves
	float lookingInputReturnToDefaultTime = 1.0f;				// If set to < 0.0, this variable flags to reset the camera with a smoothstep
	glm::vec2 lookingInputReturnToDefaultCachedFromInput;
	glm::vec2 lookingInputReturnToDefaultCachedToInput;

	// TODO: tune these lol (jk jk)
	// And then after, we can make these all consts!!!!
	float groundAcceleration = 1.4f;
	float groundDecceleration = 10.0f;
	float airAcceleration = 2.5f;
	float runSpeed = 0.75f;						// TODO: may wanna change that variable name eh
	float groundRunSpeedCantTurn = 2.5f;				// This is the run speed at which you can't turn anymore.
	float jumpSpeed[2] = { 1.325f, 1.0f };		// 1.0: 1b; 1.325: 2b; 1.585: 3b; 1.8: 4b; 2.04: 5b; 2.23: 6b; 2.39: 7b; 2.55: 8b; 2.69: 9b; 2.85: 10b;
	float jumpCoyoteTime = 0.25f;
	float jumpCoyoteTimer;
	float jumpInputDebounceTime = 0.25f;
	float jumpInputDebounceTimer;

	float currentRunSpeed = 0.0f;						// This is the value that gets changed
	float immediateTurningRequiredSpeed = 0.1f;			// The maximum velocity you can have to keep the ability to immediately turn (while grounded)

	float leanLerpTime = 10.0f;
	float leanMultiplier = 0.1f;						// NOTE: this is one over the number of degrees of delta required to get the maximum lean (e.g. 1/90degrees = 0.011f)

	glm::vec2 facingDirection = glm::vec2(0, 1);		// NOTE: this is assumed to always be normalized
	float groundedFacingTurnSpeed = 400.0f;
	float airBourneFacingTurnSpeed = 100.0f;			// Much slower than facingTurnSpeed

	float modelOffsetY = -3.35f;
	float animationSpeed = 42.0f;

private:
	void refreshResources();

	void processMovement();
	physx::PxVec3 processGroundedMovement(const glm::vec2& movementVector);
	physx::PxVec3 processAirMovement(const glm::vec2& movementVector);

	void processActions();

	void processAnimation();

	VirtualCamera playerCamera;
	bool useIndoorCamera;
	float indoorOverlapCheckRadius = 0.75f;
	float indoorOverlapCheckHeight = 17.5f;		// NOTE: very tall, because now I want false positives for indoors so I can see how to improve this algorithm later. Thus, @Incomplete
	float indoorOverlapCheckOffY = 20.0f;

	//
	// Animation Variables
	//
	int animationState = 0;		// 0:Standing	1:Jumping	2:Landing
	int prevAnimState = -1;
	bool triggerAnimationStateReset = false;

	float targetCharacterLeanValue = 0.0f;
	float characterLeanValue = 0.0f;		// [-1, 1], where 0 is no lean
	bool isMoving = false;
	bool waitUntilAnimationFinished = false;

	bool lockFacingDirection = false;
	bool lockJumping = false;

	bool triggerDrinkWaterAnimation = false;

	//
	// Rope Simulations
	//
	RopeSimulation leftSideburn, rightSideburn, backAttachment;

	//
	// HUMAN transformation
	//
	int human_numJumpsCurrent = 0;
};
