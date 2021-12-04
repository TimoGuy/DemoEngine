#include "PlayerCharacter.h"

#include <cmath>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include "components/PhysicsComponents.h"
#include "../mainloop/MainLoop.h"
#include "../render_engine/render_manager/RenderManager.h"
#include "../render_engine/resources/Resources.h"
#include "../utils/PhysicsUtils.h"
#include "../utils/GameState.h"
#include "../utils/InputManager.h"
#include "../utils/Messages.h"

#ifdef _DEBUG
#include "../imgui/imgui.h"
#include "../imgui/imgui_stdlib.h"
#endif


PlayerCharacter::PlayerCharacter()
{
	name = "Player Controller";

	physicsComponent = new PlayerPhysics(this);
	renderComponent = new PlayerRender(this);
}

void PlayerCharacter::loadPropertiesFromJson(nlohmann::json& object)		// @Override
{
	BaseObject::loadPropertiesFromJson(object["baseObject"]);

	//
	// I'll take the leftover tokens then
	//
}

nlohmann::json PlayerCharacter::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();

	return j;
}

PlayerRender::PlayerRender(BaseObject* bo) : RenderComponent(bo), bottleModelMatrix(PhysicsUtils::createGLMTransform(glm::vec3(-1.318f, 2.408f, 0.765f), glm::vec3(-6.84f, 0.0f, -150.4f))), bottleHandModelMatrix(PhysicsUtils::createGLMTransform(glm::vec3(0.015f, 4.372f, 0.01f), glm::vec3(0, 90.0f, -180.0f)))
{
	refreshResources();

	playerCamera.priority = 10;
	MainLoop::getInstance().camera.addVirtualCamera(&playerCamera);
}

PlayerRender::~PlayerRender()
{
	MainLoop::getInstance().camera.removeVirtualCamera(&playerCamera);
}

void PlayerRender::refreshResources()
{
	//
	// @IMPLEMENTATION: NOTES FOR SLIME GIRL BODY SHADER IMPLEMENTATION
	// 
	// - Make the slime do screen space refraction (For now maybe not, but just a passthru effect, but still sample from color buffer instead of doing completely opaque stuff)
	// - Keep the drawing of opaque materials onto a duplicated color buffer
	// - Draw the opaque material render in places where a fresnel shader would be white
	// - :::::LATER STUFF:::::
	// - Do the screen space refraction (this is only slight, so if it's not worth it don't do it)
	//

	//
	// Model and animation loading
	// (NOTE: it's highly recommended to only use the glTF2 format for 3d models,
	// since Assimp's model loader incorrectly includes bones and vertices with fbx)
	//
	bool recreateAnimations;
	model = (Model*)Resources::getResource("model;slimeGirl", model, &recreateAnimations);
	if (recreateAnimations)
	{
		animator =
			Animator(
				&model->getAnimations(),
				{
					"Hair Sideburn1.R",
					"Hair Sideburn2.R",
					"Hair Sideburn3.R",
					"Hair Sideburn4.R",
					"Hair Sideburn1.L",
					"Hair Sideburn2.L",
					"Hair Sideburn3.L",
					"Hair Sideburn4.L",

					"Back Attachment",
					"Hand Attachment"
				}
		);

		materials["BeltAccent"] = (Material*)Resources::getResource("material;pbrSlimeBeltAccent");
		materials["Body"] = (Material*)Resources::getResource("material;pbrSlimeBody");
		materials["Tights"] = (Material*)Resources::getResource("material;pbrSlimeTights");
		materials["Sweater"] = (Material*)Resources::getResource("material;pbrSlimeSweater");
		materials["Sweater2"] = (Material*)Resources::getResource("material;pbrSlimeSweater2");
		materials["Vest"] = (Material*)Resources::getResource("material;pbrSlimeVest");
		materials["Shorts"] = (Material*)Resources::getResource("material;pbrSlimeShorts");
		materials["Belt"] = (Material*)Resources::getResource("material;pbrSlimeBelt");
		materials["Eyebrow"] = (Material*)Resources::getResource("material;pbrSlimeEyebrow");
		materials["Eyes"] = (Material*)Resources::getResource("material;pbrSlimeEye");
		materials["Hair"] = (Material*)Resources::getResource("material;pbrSlimeHair");
		materials["Shoes"] = (Material*)Resources::getResource("material;pbrSlimeShoeWhite");
		materials["ShoeWhite2"] = (Material*)Resources::getResource("material;pbrSlimeShoeWhite2");
		materials["ShoeBlack"] = (Material*)Resources::getResource("material;pbrSlimeShoeBlack");
		materials["ShoeAccent"] = (Material*)Resources::getResource("material;pbrSlimeShoeAccent");
		materials["PlasticCap"] = (Material*)Resources::getResource("material;pbrSlimeVest");

		materials["Sweater"]->setTilingAndOffset(glm::vec4(0.4, 0.4, 0, 0));
		materials["Vest"]->setTilingAndOffset(glm::vec4(0.6, 0.6, 0, 0));
		materials["Shoes"]->setTilingAndOffset(glm::vec4(0.5, 0.5, 0, 0));

		model->setMaterials(materials);
	}

	bottleModel = (Model*)Resources::getResource("model;weaponBottle", bottleModel, &recreateAnimations);
	if (recreateAnimations)
	{
		// Now for the bottle model!
		bottleModelMaterials["PlasticCap"] = (Material*)Resources::getResource("material;pbrSlimeVest");
		bottleModelMaterials["SeeThruRubber"] = (Material*)Resources::getResource("material;pbrSlimeEye");
		bottleModelMaterials["MetalStand"] = (Material*)Resources::getResource("material;pbrRustyMetal");
		bottleModelMaterials["Straw"] = (Material*)Resources::getResource("material;pbrSlimeTights");

		bottleModel->setMaterials(bottleModelMaterials);
	}
}

void PlayerRender::processMovement()
{
	//
	// Get looking input
	//
	lookingInput += glm::vec2(InputManager::getInstance().rightStickX, InputManager::getInstance().rightStickY) * lookingSensitivity;
	lookingInput.x = fmodf(lookingInput.x, 360.0f);
	if (lookingInput.x < 0.0f)
		lookingInput.x += 360.0f;
	lookingInput.y = std::clamp(lookingInput.y, -1.0f, 1.0f);

	//
	// Movement
	//
	glm::vec2 movementVector(InputManager::getInstance().leftStickX, InputManager::getInstance().leftStickY);

	// Change input vector to camera view
	glm::vec3 ThreeDMvtVector =
		movementVector.x * glm::normalize(glm::cross(playerCamera.orientation, playerCamera.up)) +
		movementVector.y * glm::normalize(glm::vec3(playerCamera.orientation.x, 0.0f, playerCamera.orientation.z));
	movementVector.x = ThreeDMvtVector.x;
	movementVector.y = ThreeDMvtVector.z;

	if (glm::length2(movementVector) > 0.001f)
		movementVector = PhysicsUtils::clampVector(movementVector, 0.0f, 1.0f);

#ifdef _DEBUG
	// Remove input if not playmode
	if (!MainLoop::getInstance().playMode)
	{
		lookingInput = glm::vec2(0, 0);
		currentRunSpeed = 0.0f;
		facingDirection = glm::vec2(0, 1);
		movementVector = glm::vec2(0, 0);
	}
#endif

	//
	// Update playercam pos
	//
	const glm::vec3 cameraPointingToPosition = PhysicsUtils::getPosition(baseObject->getTransform()) + glm::vec3(playerCamOffset.x, playerCamOffset.y, 0);
	float cameraDistance = playerCamOffset.z;

	// Do raycast to see what the camera distance should be
	{
		physx::PxRaycastBuffer hitInfo;
		const float hitDistancePadding = 0.75f;

		if (PhysicsUtils::raycast(PhysicsUtils::toPxVec3(cameraPointingToPosition), PhysicsUtils::toPxVec3(-playerCamera.orientation), std::abs(cameraDistance) + hitDistancePadding, hitInfo))
		{
			cameraDistance = -hitInfo.block.distance + hitDistancePadding;		// NOTE: must be negative distance since behind model
		}

		//
		// Update max distance member var
		//
		if (currentMaxCamDistance <= cameraDistance)
		{
			// Reset timer
			currentMaxCamDistance = cameraDistance;
			maxCamDistanceHoldTimer = maxCamDistanceHoldTime;
		}
		else
		{
			// Slowly move back once timer is down
			maxCamDistanceHoldTimer -= MainLoop::getInstance().deltaTime;
			if (maxCamDistanceHoldTimer < 0)
			{
				currentMaxCamDistance =
					PhysicsUtils::moveTowards(currentMaxCamDistance, cameraDistance, maxCamDistanceShiftSpeed * MainLoop::getInstance().deltaTime);
			}
		}
	}

	// Setup camera orientation
	glm::quat lookingRotation(glm::radians(glm::vec3(lookingInput.y * 85.0f, -lookingInput.x, 0.0f)));
	playerCamera.orientation = lookingRotation * glm::vec3(0, 0, 1);


	const glm::vec3 depthOffset(0, 0, currentMaxCamDistance);
	playerCamera.position = cameraPointingToPosition + lookingRotation * depthOffset;

	//
	// Update movement
	//
	targetCharacterLeanValue = 0.0f;
	isMoving = false;

	physx::PxVec3 velocity(0.0f);
	if (((PlayerPhysics*)baseObject->getPhysicsComponent())->getIsGrounded())
		velocity = processGroundedMovement(movementVector);
	else
		velocity = processAirMovement(movementVector);

	if (isMoving)
		GameState::getInstance().inputStaminaEvent(StaminaEvent::MOVE, MainLoop::getInstance().deltaTime);

	characterLeanValue = PhysicsUtils::lerp(
		characterLeanValue,
		targetCharacterLeanValue,
		leanLerpTime[GameState::getInstance().playerIsHoldingWater] * MainLoop::getInstance().deltaTime
	);

	((PlayerPhysics*)baseObject->getPhysicsComponent())->velocity = velocity;

	model->localTransform =
		glm::translate(glm::mat4(1.0f), glm::vec3(0, modelOffsetY, 0)) *
		glm::eulerAngleXYZ(0.0f, std::atan2f(facingDirection.x, facingDirection.y), glm::radians(characterLeanValue * 20.0f)) *
		glm::scale(glm::mat4(1.0f), PhysicsUtils::getScale(baseObject->getTransform()));
}

physx::PxVec3 PlayerRender::processGroundedMovement(const glm::vec2& movementVector)
{
	bool isCarryingWater = GameState::getInstance().playerIsHoldingWater;

	//
	// Set Movement Vector
	//
	const float movementVectorLength = glm::length(movementVector);
	if (movementVectorLength > 0.001f)
	{
		isMoving = true;

		//
		// Update facing direction
		//
		if (!lockFacingDirection)
		{
			physx::PxVec3 velocityCopy = ((PlayerPhysics*)baseObject->getPhysicsComponent())->velocity;
			velocityCopy.y = 0.0f;

			// Skid stop behavior:
			float mvtDotFacing = glm::dot(movementVector, facingDirection);
			if (velocityCopy.magnitude() > immediateTurningRequiredSpeed && mvtDotFacing < -0.707106781f)
			{
				facingDirection = movementVector;
				currentRunSpeed = 0;
			}
			// Immediate facing direction when moving slowly:
			else if (velocityCopy.magnitude() <= immediateTurningRequiredSpeed)
			{
				facingDirection = movementVector;
			}
			// Normal facing behavior:
			else
			{
				//
				// Slowly face towards the targetfacingdirection
				//
				float facingDirectionAngle = glm::degrees(std::atan2f(facingDirection.x, facingDirection.y));
				float targetDirectionAngle = glm::degrees(std::atan2f(movementVector.x, movementVector.y));

				float newFacingDirectionAngle = glm::radians(PhysicsUtils::moveTowardsAngle(facingDirectionAngle, targetDirectionAngle, facingTurnSpeed[isCarryingWater] * MainLoop::getInstance().deltaTime));

				facingDirection = glm::vec2(std::sinf(newFacingDirectionAngle), std::cosf(newFacingDirectionAngle));

				//
				// Calculate lean amount (use targetDirectionAngle bc of lack of deltaTime mess)
				//
				float deltaFacingDirectionAngle = targetDirectionAngle - facingDirectionAngle;

				if (deltaFacingDirectionAngle < -180.0f)			deltaFacingDirectionAngle += 360.0f;
				else if (deltaFacingDirectionAngle > 180.0f)		deltaFacingDirectionAngle -= 360.0f;

				const float deadZoneDeltaAngle = 0.1f;
				if (std::abs(deltaFacingDirectionAngle) > deadZoneDeltaAngle)
					targetCharacterLeanValue = std::clamp(-deltaFacingDirectionAngle * leanMultiplier[isCarryingWater], -1.0f, 1.0f);
				else
					targetCharacterLeanValue = 0.0f;
			}
		}
	}

	//
	// Update Running Speed
	//
	float targetRunningSpeed = movementVectorLength * groundRunSpeed[isCarryingWater];
	currentRunSpeed = PhysicsUtils::moveTowards(
		currentRunSpeed,
		targetRunningSpeed,
		(currentRunSpeed < targetRunningSpeed ? groundAcceleration[isCarryingWater] : groundDecceleration[isCarryingWater]) *
		MainLoop::getInstance().deltaTime
	);

	//
	// Apply the maths onto the actual velocity vector now!
	//
	glm::vec3 velocity = glm::vec3(facingDirection.x, 0, facingDirection.y) * currentRunSpeed;

	if (((PlayerPhysics*)baseObject->getPhysicsComponent())->getIsSliding() || ((PlayerPhysics*)baseObject->getPhysicsComponent())->getIsCeilingSliding())
	{
		// Cut off movement towards uphill if supposed to be sliding
		glm::vec3 normal = ((PlayerPhysics*)baseObject->getPhysicsComponent())->getGroundedNormal();
		glm::vec3 TwoDNormal = glm::normalize(glm::vec3(normal.z, 0.0f, -normal.x));		// Flip positions to get the 90 degree right vector
		velocity = glm::dot(TwoDNormal, velocity) * TwoDNormal;								// Project the velocity vector onto the 90 degree flat vector;
	}
	else
	{
		// Add on a grounded rotation based on the ground you're standing on (!isSliding)
		glm::quat slopeRotation = glm::rotation(
			glm::vec3(0, 1, 0),
			((PlayerPhysics*)baseObject->getPhysicsComponent())->getGroundedNormal()
		);
		velocity = slopeRotation * velocity;
	}

	//
	// Fetch in physics y (overwriting)
	// (HOWEVER, not if going down a slope's -y is lower than the reported y (ignore when going up/jumping))
	//
	if (velocity.y < 0.0f)
		velocity.y = std::min(((PlayerPhysics*)baseObject->getPhysicsComponent())->velocity.y, velocity.y);
	else
		velocity.y = ((PlayerPhysics*)baseObject->getPhysicsComponent())->velocity.y;

	// Jump (but not if sliding)
	if (!lockJumping &&
		!((PlayerPhysics*)baseObject->getPhysicsComponent())->getIsSliding() &&
		InputManager::getInstance().jumpPressed)
	{
		velocity.y = jumpSpeed[isCarryingWater];
		GameState::getInstance().inputStaminaEvent(StaminaEvent::JUMP);
	}

	return physx::PxVec3(velocity.x, velocity.y, velocity.z);
}

physx::PxVec3 PlayerRender::processAirMovement(const glm::vec2& movementVector)
{
	bool isCarryingWater = GameState::getInstance().playerIsHoldingWater;

	//
	// Update facing direction
	//
	if (glm::length2(movementVector) > 0.001f)
	{
		isMoving = true;

		float facingDirectionAngle = glm::degrees(std::atan2f(facingDirection.x, facingDirection.y));
		float targetDirectionAngle = glm::degrees(std::atan2f(movementVector.x, movementVector.y));

		facingDirectionAngle = glm::radians(PhysicsUtils::moveTowardsAngle(facingDirectionAngle, targetDirectionAngle, airBourneFacingTurnSpeed[isCarryingWater] * MainLoop::getInstance().deltaTime));

		facingDirection = glm::vec2(std::sinf(facingDirectionAngle), std::cosf(facingDirectionAngle));
	}

	//
	// Setup velocity manipulation
	//
	physx::PxVec3 currentVelocity = ((PlayerPhysics*)baseObject->getPhysicsComponent())->velocity;

	//
	// Just add velocity in a flat way
	//
	float accelAmount = glm::length(movementVector);		// [0,1] range due to clamping before this function
	currentVelocity.x += movementVector.x * accelAmount * airAcceleration[isCarryingWater] * MainLoop::getInstance().deltaTime;
	currentVelocity.z += movementVector.y * accelAmount * airAcceleration[isCarryingWater] * MainLoop::getInstance().deltaTime;

	//
	// Clamp the velocity
	//
	glm::vec2 flatNewVelocity(currentVelocity.x, currentVelocity.z);
	if (glm::length2(flatNewVelocity) > 0.001f)
	{
		flatNewVelocity = PhysicsUtils::clampVector(flatNewVelocity, 0.0f, groundRunSpeed[isCarryingWater]);		// NOTE: groundRunSpeed is simply a placeholder... I think.
		currentVelocity.x = flatNewVelocity.x;
		currentVelocity.z = flatNewVelocity.y;
	}

	return currentVelocity;
}

void PlayerRender::processActions()
{
	if (GameState::getInstance().playerIsHoldingWater &&
		InputManager::getInstance().useItemPressed)
	{
		// Drink the water
		GameState::getInstance().playerIsHoldingWater = false;
		GameState::getInstance().currentPlayerStaminaAmount =
			GameState::getInstance().maxPlayerStaminaAmount;

		triggerDrinkWaterAnimation = true;
	}
}

float hairWeightMult = 10.0f;				// @Debug
void PlayerRender::processAnimation()
{
	//
	// Process movement into animationstates
	//
	if (animationState == 0)
	{
		if (!((PlayerPhysics*)baseObject->getPhysicsComponent())->getIsGrounded())
			// Jump
			animationState = 1;
	}
	else if (animationState == 1)
	{
		if (((PlayerPhysics*)baseObject->getPhysicsComponent())->getIsGrounded() && !((PlayerPhysics*)baseObject->getPhysicsComponent())->getIsSliding())
			// Landing
			animationState = 2;
	}
	else if (animationState == 2)
	{
		if (!((PlayerPhysics*)baseObject->getPhysicsComponent())->getIsGrounded())
			// Jump
			animationState = 1;
		else
		{
			// Wait until land animation is finished
			if (animator.isAnimationFinished(4, MainLoop::getInstance().deltaTime * animationSpeed) ||
				animator.isAnimationFinished(5, MainLoop::getInstance().deltaTime * animationSpeed))
			{
				// Standing
				animationState = 0;
				//std::cout << "Switched to standing" << std::endl;
			}
		}
	}
	else if (animationState == 3)
	{
		// Lock movement
		((PlayerPhysics*)baseObject->getPhysicsComponent())->lockVelocity(false);
		lockFacingDirection = true;
		lockJumping = true;

		// Wait until draw water animation is finished
		if (animator.isAnimationFinished(8, MainLoop::getInstance().deltaTime * animationSpeed))
		{
			// Pick up bottle
			animationState = 5;
		}
	}
	else if (animationState == 4)
	{
		// Lock movement
		((PlayerPhysics*)baseObject->getPhysicsComponent())->lockVelocity(false);
		lockFacingDirection = true;
		lockJumping = true;

		// Wait until drink-water animation is finished
		if (animator.isAnimationFinished(9, MainLoop::getInstance().deltaTime * animationSpeed))
		{
			// Pick up bottle
			animationState = 5;
		}
	}
	else if (animationState == 5)
	{
		// Lock movement
		((PlayerPhysics*)baseObject->getPhysicsComponent())->lockVelocity(false);
		lockFacingDirection = true;
		lockJumping = true;

		// Wait until pick-up-bottle animation is finished
		if (animator.isAnimationFinished(10, MainLoop::getInstance().deltaTime * animationSpeed))
		{
			// Standing
			animationState = 0;
			lockFacingDirection = false;
			lockJumping = false;
		}
	}

	if (Messages::getInstance().checkForMessage("PlayerCollectWater"))
	{
		animationState = 3;
	}

	if (triggerDrinkWaterAnimation)
	{
		triggerDrinkWaterAnimation = false;
		animationState = 4;
	}

	//
	// Update Animation State
	//
	if (prevAnimState != animationState)
	{
		switch (animationState)
		{
		case 0:
			// Idle/Move
			animator.playAnimation((unsigned int)isMoving, 6.0f, true, true);
			break;

		case 1:
			// Jump
			animator.playAnimation(2 + (int)isMoving, 0.0f, false, true);
			break;

		case 2:
			// Land
			animator.playAnimation(4 + (int)isMoving, 0.0f, false, true);
			break;

		case 3:
			// Draw water
			animator.playAnimation(8, 1.0f, false, true);
			break;

		case 4:
			// Drink water
			animator.playAnimation(9, 1.0f, false, true);
			break;

		case 5:
			// Pick up bottle
			animator.playAnimation(10, 7.5f, false, true);
			break;

		default:
			std::cout << "Animation State " << animationState << " not recognized." << std::endl;
			break;
		}
	}
	else if (animationState == 0)
		// Idle/Move
		animator.playAnimation((unsigned int)isMoving, 6.0f);

	prevAnimState = animationState;


	//
	// Mesh Skinning
	// @Optimize: This line (takes "less than 7ms"), if run multiple times, will bog down performance like crazy. Perhaps implement gpu-based animation???? Or maybe optimize this on the cpu side.
	//
	animator.updateAnimation(MainLoop::getInstance().deltaTime * animationSpeed);		// Correction: this adds more than 10ms consistently

	//
	// @TODO: Do IK (Forward and Backward Reaching Inverse Kinematics for a heuristic approach)
	//
#ifdef _DEBUG
	if (MainLoop::getInstance().playMode)
#endif
	{
		if (rightSideburn.isFirstTime || leftSideburn.isFirstTime || backAttachment.isFirstTime)
		{
			//
			// Setup Rope simulations
			//
			std::vector<glm::vec3> rightSideburnPoints;
			const glm::vec4 neutralPosition(0, 0, 0, 1);
			rightSideburnPoints.push_back(model->localTransform * animator.getBoneTransformation("Hair Sideburn1.R").globalTransformation * neutralPosition);
			rightSideburnPoints.push_back(model->localTransform * animator.getBoneTransformation("Hair Sideburn2.R").globalTransformation * neutralPosition);
			rightSideburnPoints.push_back(model->localTransform * animator.getBoneTransformation("Hair Sideburn3.R").globalTransformation * neutralPosition);
			rightSideburnPoints.push_back(model->localTransform * animator.getBoneTransformation("Hair Sideburn4.R").globalTransformation * neutralPosition);
			rightSideburn.initializePoints(rightSideburnPoints);

			std::vector<glm::vec3> leftSideburnPoints;
			leftSideburnPoints.push_back(model->localTransform * animator.getBoneTransformation("Hair Sideburn1.L").globalTransformation * neutralPosition);
			leftSideburnPoints.push_back(model->localTransform * animator.getBoneTransformation("Hair Sideburn2.L").globalTransformation * neutralPosition);
			leftSideburnPoints.push_back(model->localTransform * animator.getBoneTransformation("Hair Sideburn3.L").globalTransformation * neutralPosition);
			leftSideburnPoints.push_back(model->localTransform * animator.getBoneTransformation("Hair Sideburn4.L").globalTransformation * neutralPosition);
			leftSideburn.initializePoints(leftSideburnPoints);

			std::vector<glm::vec3> backAttachmentPoints;
			backAttachmentPoints.push_back(model->localTransform * animator.getBoneTransformation("Back Attachment").globalTransformation * neutralPosition);
			backAttachmentPoints.push_back(model->localTransform * animator.getBoneTransformation("Back Attachment").globalTransformation * (neutralPosition + glm::vec4(0, -50, 0, 0)));
			backAttachment.initializePoints(backAttachmentPoints);
			backAttachment.limitTo45degrees = true;
			
			rightSideburn.isFirstTime = false;
			leftSideburn.isFirstTime = false;
			backAttachment.isFirstTime = false;
		}
		else
		{
			//
			// Just reset/lock the first bone
			//
			static const glm::vec4 neutralPosition(0, 0, 0, 1);
			leftSideburn.setPointPosition(0, model->localTransform * animator.getBoneTransformation("Hair Sideburn1.L").globalTransformation * neutralPosition);
			rightSideburn.setPointPosition(0, model->localTransform * animator.getBoneTransformation("Hair Sideburn1.R").globalTransformation * neutralPosition);
			backAttachment.setPointPosition(0, model->localTransform * animator.getBoneTransformation("Back Attachment").globalTransformation * neutralPosition);
			
			leftSideburn.simulateRope(hairWeightMult);
			rightSideburn.simulateRope(hairWeightMult);
			backAttachment.simulateRope(850.0f);

			//
			// Do ik calculations for sideburns
			//
			const glm::mat4 inverseRenderTransform = glm::inverse(model->localTransform);

			const glm::vec3 r0 = inverseRenderTransform * glm::vec4(rightSideburn.getPoint(0), 1);
			const glm::vec3 r1 = inverseRenderTransform * glm::vec4(rightSideburn.getPoint(1), 1);
			const glm::vec3 r2 = inverseRenderTransform * glm::vec4(rightSideburn.getPoint(2), 1);
			const glm::vec3 r3 = inverseRenderTransform * glm::vec4(rightSideburn.getPoint(3), 1);

			const glm::vec3 l0 = inverseRenderTransform * glm::vec4(leftSideburn.getPoint(0), 1);
			const glm::vec3 l1 = inverseRenderTransform * glm::vec4(leftSideburn.getPoint(1), 1);
			const glm::vec3 l2 = inverseRenderTransform * glm::vec4(leftSideburn.getPoint(2), 1);
			const glm::vec3 l3 = inverseRenderTransform * glm::vec4(leftSideburn.getPoint(3), 1);

			const glm::vec3 b0 = inverseRenderTransform * glm::vec4(backAttachment.getPoint(0), 1);
			const glm::vec3 b1 = inverseRenderTransform * glm::vec4(backAttachment.getPoint(1), 1);

			const glm::quat rotation180(glm::radians(glm::vec3(180, 0, 0)));

			glm::quat rotation;
			rotation = glm::quat(glm::vec3(0, -1, 0), glm::normalize(r1 - r0));
			animator.setBoneTransformation("Hair Sideburn1.R", glm::translate(glm::mat4(1.0f), r0) * glm::toMat4(rotation * rotation180));
			rotation = glm::quat(glm::vec3(0, -1, 0), glm::normalize(r2 - r1));
			animator.setBoneTransformation("Hair Sideburn2.R", glm::translate(glm::mat4(1.0f), r1) * glm::toMat4(rotation * rotation180));
			rotation = glm::quat(glm::vec3(0, -1, 0), glm::normalize(r3 - r2));
			animator.setBoneTransformation("Hair Sideburn3.R", glm::translate(glm::mat4(1.0f), r2) * glm::toMat4(rotation * rotation180));
			animator.setBoneTransformation("Hair Sideburn4.R", glm::translate(glm::mat4(1.0f), r3) * glm::toMat4(rotation * rotation180));

			rotation = glm::quat(glm::vec3(0, -1, 0), glm::normalize(l1 - l0));
			animator.setBoneTransformation("Hair Sideburn1.L", glm::translate(glm::mat4(1.0f), l0) * glm::toMat4(rotation * rotation180));
			rotation = glm::quat(glm::vec3(0, -1, 0), glm::normalize(l2 - l1));
			animator.setBoneTransformation("Hair Sideburn2.L", glm::translate(glm::mat4(1.0f), l1) * glm::toMat4(rotation * rotation180));
			rotation = glm::quat(glm::vec3(0, -1, 0), glm::normalize(l3 - l2));
			animator.setBoneTransformation("Hair Sideburn3.L", glm::translate(glm::mat4(1.0f), l2) * glm::toMat4(rotation * rotation180));
			animator.setBoneTransformation("Hair Sideburn4.L", glm::translate(glm::mat4(1.0f), l3) * glm::toMat4(rotation * rotation180));

			rotation = glm::quat(glm::vec3(0, -1, 0), glm::normalize(b1 - b0));
			animator.setBoneTransformation("Back Attachment", glm::translate(glm::mat4(1.0f), b0) * glm::toMat4(rotation * rotation180));
		}
	}

	//
	// Calculate the bottle transformation matrix
	// @@@TODO: fix the baseObject->getTransform() areas, bc the transformation hierarchy isn't established yet.
	if (animationState == 3 || animationState == 4 || animationState == 5)
		bottleModel->localTransform = model->localTransform * animator.getBoneTransformation("Hand Attachment").globalTransformation * bottleHandModelMatrix;
	else
		bottleModel->localTransform = model->localTransform * animator.getBoneTransformation("Back Attachment").globalTransformation * bottleModelMatrix;

	prevIsGrounded = ((PlayerPhysics*)baseObject->getPhysicsComponent())->getIsGrounded();
}

PlayerCharacter::~PlayerCharacter()
{
	delete renderComponent;
	delete physicsComponent;
}

void PlayerCharacter::preRenderUpdate()
{
	processMovement();
	processActions();
	processAnimation();
}

void PlayerRender::render()
{
#ifdef _DEBUG
	refreshResources();
#endif

	//
	// Update the animation bones in the render manager
	//
	std::vector<glm::mat4>* boneTransforms = animator.getFinalBoneMatrices();
	MainLoop::getInstance().renderManager->updateSkeletalBonesUBO(boneTransforms);
	model->render(baseObject->getTransform(), 0);
	bottleModel->render(baseObject->getTransform(), 0);
}

void PlayerRender::renderShadow(GLuint programId)
{
#ifdef _DEBUG
	refreshResources();
#endif

	std::vector<glm::mat4>* boneTransforms = animator.getFinalBoneMatrices();
	MainLoop::getInstance().renderManager->updateSkeletalBonesUBO(boneTransforms);
	model->render(baseObject->getTransform(), programId);
	bottleModel->render(baseObject->getTransform(), programId);
}

#ifdef _DEBUG
void PlayerCharacter::imguiPropertyPanel()
{
	ImGui::InputText("Name", &name);
	ImGui::Separator();

	// @Broken: The commented out lines below are apparently illegal operations towards the physics component
	PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(getTransform()));
	glm::vec3 newPos = PhysicsUtils::getPosition(getTransform());
	((PlayerPhysics*)getPhysicsComponent())->controller->setPosition(physx::PxExtendedVec3(newPos.x, newPos.y, newPos.z));

	ImGui::DragFloat3("Controller up direction", &((PlayerPhysics*)getPhysicsComponent())->tempUp[0]);
	((PlayerPhysics*)getPhysicsComponent())->controller->setUpDirection(((PlayerPhysics*)getPhysicsComponent())->tempUp.getNormalized());

	ImGui::Separator();
	ImGui::Text("Virtual Camera");
	ImGui::DragFloat3("VirtualCamPosition", &((PlayerRender*)getRenderComponent())->playerCamOffset[0]);
	ImGui::DragFloat2("Looking Input", &((PlayerRender*)getRenderComponent())->lookingInput[0]);
	ImGui::DragFloat2("Looking Sensitivity", &((PlayerRender*)getRenderComponent())->lookingSensitivity[0]);

	ImGui::Separator();
	ImGui::Text("Movement Properties");
	ImGui::DragFloat2("Running Acceleration", &((PlayerRender*)getRenderComponent())->groundAcceleration[0], 0.1f);
	ImGui::DragFloat2("Running Decceleration", &((PlayerRender*)getRenderComponent())->groundDecceleration[0], 0.1f);
	ImGui::DragFloat2("Running Acceleration (Air)", &((PlayerRender*)getRenderComponent())->airAcceleration[0], 0.1f);
	ImGui::DragFloat2("Running Speed", &((PlayerRender*)getRenderComponent())->groundRunSpeed[0], 0.1f);
	ImGui::DragFloat2("Jump Speed", &((PlayerRender*)getRenderComponent())->jumpSpeed[0], 0.1f);
	ImGui::Text(("Facing Direction: (" + std::to_string(((PlayerRender*)getRenderComponent())->facingDirection.x) + ", " + std::to_string(((PlayerRender*)getRenderComponent())->facingDirection.y) + ")").c_str());
	ImGui::DragFloat2("Leaning Lerp Time", &((PlayerRender*)getRenderComponent())->leanLerpTime[0]);
	ImGui::DragFloat2("Lean Multiplier", &((PlayerRender*)getRenderComponent())->leanMultiplier[0], 0.05f);
	ImGui::DragFloat2("Facing Movement Speed", &((PlayerRender*)getRenderComponent())->facingTurnSpeed[0], 0.1f);
	ImGui::DragFloat2("Facing Movement Speed (Air)", &((PlayerRender*)getRenderComponent())->airBourneFacingTurnSpeed[0], 0.1f);

	ImGui::Separator();
	ImGui::DragFloat("Model Offset Y", &((PlayerRender*)getRenderComponent())->modelOffsetY, 0.05f);
	ImGui::DragFloat("Model Animation Speed", &((PlayerRender*)getRenderComponent())->animationSpeed);

	ImGui::Separator();
	ImGui::ColorPicker3("Body Zelly Color", &((ZellyMaterial*)((PlayerRender*)getRenderComponent())->materials["Body"])->getColor().x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
	ImGui::ColorPicker3("Hair Zelly Color", &((ZellyMaterial*)((PlayerRender*)getRenderComponent())->materials["Hair"])->getColor().x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

	ImGui::Separator();
	ImGui::DragFloat("Hair Weight", &hairWeightMult);

	// @Tune: these will mess up the matrices bc of how the imguiTransformMatrixProps() func works
	//ImGui::Separator();
	//ImGui::Text("Bottle Model Matrix");
	//PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(((PlayerRender*)baseObject->getRenderComponent())->bottleModelMatrix));
	//ImGui::Text("Bottle Hand Model Matrix");
	//PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(((PlayerRender*)baseObject->getRenderComponent())->bottleHandModelMatrix));
}

void PlayerCharacter::imguiRender()
{
}
#endif


//
// ROPESIMULATION
//
void RopeSimulation::initializePoints(const std::vector<glm::vec3>& points)
{
	RopeSimulation::points = RopeSimulation::prevPoints = points;

	size_t counter = 0;
	for (size_t i = 0; i < points.size() - 1; i++)
	{
		distances.push_back(glm::length(points[i] - points[i + 1]));
	}
}

void RopeSimulation::setPointPosition(size_t index, const glm::vec3& position)
{
	glm::vec3 deltaMovement = position - points[index];
	points[index] = position;

	// NOTE: this is for alleviating glitchyness when point moves too much.
	// This code propogates the movement from this new set operation to the other points of the rope
	for (size_t i = 0; i < prevPoints.size(); i++)
		prevPoints[i] += deltaMovement;
}

void RopeSimulation::simulateRope(float gravityMultiplier)
{
	const float deltaTime = MainLoop::getInstance().deltaTime;

	//
	// Part 1: cycle thru all points (except #0 is locked, so skip that one), and update their positions
	//
	for (size_t i = 1; i < points.size(); i++)
	{
		glm::vec3 savedPoint = points[i];
		points[i] += (points[i] - prevPoints[i]) + glm::vec3(0, -9.8f * gravityMultiplier * deltaTime * deltaTime, 0);		// TODO: figure out why the gravity term requires deltaTime * deltaTime instead of regular stuff huh
		
		// Limit to 45 degrees
		if (limitTo45degrees)
		{
			const float sin45deg = 0.707106781f;
			points[i].y = std::min(points[i].y, points[i - 1].y - distances[i - 1] * sin45deg);
		}
		
		prevPoints[i] = savedPoint;
	}

	//
	// Part 2: solve x times to try to get the distance correct again!
	//
	const int numIterations = 10;
	for (int i = 0; i < numIterations; i++)
	{
		for (size_t j = 0; j < distances.size(); j++)
		{
			//if (glm::length(points[j] - points[j + 1]) < distances[j])
			//	continue;

			glm::vec3 midpoint = (points[j] + points[j + 1]) / 2.0f;
			glm::vec3 direction = glm::normalize(points[j] - points[j + 1]);
			if (j > 0)
				points[j] = midpoint + direction * distances[j] / 2.0f;
			points[j + 1] = midpoint - direction * distances[j] / 2.0f;
		}
		for (int j = (int)distances.size() - 1; j >= 0; j--)
		{

			// NOTE: ignore dumb warnings
			glm::vec3 midpoint = (points[j] + points[j + 1]) / 2.0f;
			glm::vec3 direction = glm::normalize(points[j] - points[j + 1]);
			if (j > 0)
				points[j] = midpoint + direction * distances[j] / 2.0f;
			points[j + 1] = midpoint - direction * distances[j] / 2.0f;
		}
	}
}
