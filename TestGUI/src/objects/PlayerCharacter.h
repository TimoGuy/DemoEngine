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


struct SimpleSphere
{
	glm::vec3 origin;
	float radius;
};


enum class PlayerState
{
	NORMAL,
	WALL_CLIMB_HUMAN,	// @NOTE: this contains a short wall kick-up, and then a last-effort small jump up to get the last boost. Why not, just so the player can possibly get that ledge grab.  -Timo
	LEDGE_GRAB_HUMAN,	// @NOTE: if you press <jump> on this ledge grab, I'm not gonna do the player a favor and push them onto the platform. If they don't do it themselves, that's wasted stamina on their part. I'm not gonna babysit the player yo.  -Timo
	WALL_SCALE_CAT,		// @NOTE: @TODO: figger how this works
};


struct PlayerState_WallClimbHuman_Data
{
	bool canEnterIntoState = true;
	float climbTime = 0.15f;
	float climbTimer = 0.0f;
	float climbVelocityY = 1.0f;
	float climbDistancePadding = 1.0f;	// @NOTE: the capsule controller's radius will be added onto this
};


struct PlayerState_LedgeGrabHuman_Data
{
	float checkWallFromCenterY = 1.0f;
	float checkWallPadding = 1.0f;		// @NOTE: it's a lot of padding, but it'll be nice bc of the extra length
	float checkLedgeFromCenterY = 3.5f;		// @TODO: TUNE THESE VALUES
	float checkLedgeRayDistance = 2.5f;
	float checkLedgeTuckin = 0.1f;
	float checkLedgeCreviceHeightMin = 0.5f;
	glm::vec3 holdLedgePosition;
	glm::vec2 holdLedgeOffset{ 0.1f, -2.0f };
	float jumpSpeed = 1.2f;
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

	bool useFollowCamera = true;	// NOTE: if player is doing lookingInput.x inputs, then the follow camera is disabled for the frame this occurred.
	glm::vec3 followCameraAnchorPosition;		// NOTE: essentially it's supposed to be a flat position variable.
	float followCameraAnchorDistance = -30;		// Equivalent to outdoor cam offset. (at least at 2022-02-21)
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
	glm::vec2 groundAccelerationDeceleration = { 1.4f, 10.0f };
	glm::vec2 weaponDrawnAccelerationDeceleration = { 0.65f, 5.0f };
	float airAcceleration = 2.5f;
	float groundRunSpeed = 0.75f;
	float weaponDrawnRunSpeed = 0.4f;
	float groundRunSpeedCantTurn = 2.5f;				// This is the run speed at which you can't turn anymore.
	float jumpSpeed = 1.325f;							// Jumping height scale: 1.0: 1b; 1.325: 2b; 1.585: 3b; 1.8: 4b; 2.04: 5b; 2.23: 6b; 2.39: 7b; 2.55: 8b; 2.69: 9b; 2.85: 10b;
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

	// @NOTE: this algorithm will work quite a bit differently than the other facingTurnSpeed values.
	// This is a value that will spin the facing direction that won't be really overridable. Maybe it might be?
	// Nah, there will be a mode where this weapon-body-spinning will affect the facing direction. When this
	// doesn't have its effect anymore, then it'll revert back to the traditional grounded/airBourneFacingTurnSpeed systems.
	//					-Timo
	bool weaponDrawn = false;
	bool weaponDrawnPrevIsGrounded = false;
	int weaponDrawnStyle = 0;		// @NOTE: see @WEAPON_DRAWN_STYLE
	glm::vec2 weaponDrawnSpinSpeedMinMax = { 200.0f, 1000.0f };
	float weaponDrawnSpinAccumulated = 0.0f;			// @TODO: Make this accumulated spin amount keep going even after the player lets go of the weaponDrawn button!!!! (unless if they used the spin amount for an attack eh!)
	float weaponDrawnSpinAmountThreshold = 150.0f;		// @NOTE: this is the threshold for the prespin to go into the real spinny spinny mode
	float weaponDrawnSpinDeceleration = 100.0f;
	float weaponDrawnSpinBuildupAmount = 1000.0f;		// @NOTE: this is used in the calculation for the spinSpeedMinMaxLerpValue
	bool prevIsSpinnySpinny = false;					// @FIXME: This likely won't do much good??? Idk. This isn't the right spot for it for sure. @REFACTOR
	
	// Model Anim stuff I guess
	float modelOffsetY = -3.35f;
	float animationSpeed = 42.0f;

private:
	void refreshResources();

	void processMovement();
	physx::PxVec3 processGroundedMovement(const glm::vec2& movementVector);
	physx::PxVec3 processAirMovement(const glm::vec2& movementVector);

	PlayerState playerState = PlayerState::NORMAL;
	PlayerState_WallClimbHuman_Data ps_wallClimbHumanData;
	PlayerState_LedgeGrabHuman_Data ps_ledgeGrabHumanData;

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
	RopeSimulation leftSideburn, rightSideburn;

	//
	// Bottle AABB Construction Balls
	// https://www.gamedev.net/forums/topic/347234-quickest-way-to-compute-a-new-aabb-from-an-aabb-transform/
	//
	std::vector<SimpleSphere> aabbConstructionBalls = {		// @HARDCODED: this seemed like enough to give a much better representation of how much "stamina" is left  -Timo
		SimpleSphere{ glm::vec3(0.0f, 2.710f, 0.0f), 0.420f },
		SimpleSphere{ glm::vec3(0.0f, 0.840f, 0.0f), 0.420f }
	};
#ifdef _DEVELOP
	bool showDebugAABBConstructionBalls = false;
#endif

	//
	// @HUMAN transformation
	//
	int human_numJumpsCurrent = 0;
};
