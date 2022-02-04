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

#ifdef _DEVELOP
#include "../imgui/imgui.h"
#endif

#define REMAP(value, istart, istop, ostart, ostop) ((value) - (istart)) / ((istop) - (istart)) * ((ostop) - (ostart)) + (ostart)


PlayerCharacter::PlayerCharacter() : bottleModelMatrix(PhysicsUtils::createGLMTransform(glm::vec3(-1.318f, 2.408f, 0.765f), glm::vec3(-6.84f, 0.0f, -150.4f))), bottleHandModelMatrix(PhysicsUtils::createGLMTransform(glm::vec3(0.015f, 4.372f, 0.01f), glm::vec3(0, 90.0f, -180.0f))), useIndoorCamera(false)
{
	name = "Player Controller";

	physicsComponent = new PlayerPhysics(this);

	refreshResources();
	renderComponent = new RenderComponent(this);
	renderComponent->addModelToRender({ model, true, &animator });
	renderComponent->addModelToRender({ bottleModel, true, nullptr });

	playerCamera.priority = 10;
	MainLoop::getInstance().camera.addVirtualCamera(&playerCamera);
}

PlayerCharacter::~PlayerCharacter()
{
	delete renderComponent;
	delete physicsComponent;

	MainLoop::getInstance().camera.removeVirtualCamera(&playerCamera);
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

void PlayerCharacter::refreshResources()
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

		((PBRMaterial*)materials["Sweater"])->setTilingAndOffset(glm::vec4(0.4, 0.4, 0, 0));
		((PBRMaterial*)materials["Vest"])->setTilingAndOffset(glm::vec4(0.6, 0.6, 0, 0));
		((PBRMaterial*)materials["Shoes"])->setTilingAndOffset(glm::vec4(0.5, 0.5, 0, 0));

		model->setMaterials(materials);

		model->setDepthPriorityOfMeshesWithMaterial("Eyebrow", -0.5f);
	}

	bottleModel = (Model*)Resources::getResource("model;weaponBottle", bottleModel, &recreateAnimations);
	if (recreateAnimations)
	{
		// Now for the bottle model!
		bottleModelMaterials["PlasticCap"] = (Material*)Resources::getResource("material;pbrSlimeVest");
		bottleModelMaterials["SeeThruRubber"] = (Material*)Resources::getResource("material;pbrBottleBody");
		bottleModelMaterials["MetalStand"] = (Material*)Resources::getResource("material;pbrRustyMetal");
		bottleModelMaterials["Straw"] = (Material*)Resources::getResource("material;pbrSlimeTights");
		bottleModelMaterials["Water"] = (Material*)Resources::getResource("material;containedWater");

		bottleModel->setMaterials(bottleModelMaterials);
		bottleModel->setDepthPriorityOfMeshesWithMaterial("SeeThruRubber", 0.0f);
		//bottleModel->setDepthPriorityOfMeshesWithMaterial("Water", 1.0f);		// Make Water render before SeeThruRubber
		bottleModel->setDepthPriorityOfMeshesWithMaterial("Water", (GameState::getInstance().playerIsHoldingWater ? 1.0f : -1.0f));		// @HACK: shouldn't be using transparency ordering hax like this
	}
}

constexpr float minMvtVectorMagnitude = 0.35f;
void PlayerCharacter::processMovement()
{
	//
	// Get looking input
	//
	if (lookingInputReturnToDefaultTime >= 1.0f)
	{
		lookingInput += glm::vec2(InputManager::getInstance().rightStickX, InputManager::getInstance().rightStickY) * lookingSensitivity;
		lookingInput.x = fmodf(lookingInput.x, 360.0f);
		if (lookingInput.x < 0.0f)
			lookingInput.x += 360.0f;
		lookingInput.y = std::clamp(lookingInput.y, -1.0f, 1.0f);

		// Reset to default if reset camera button pressed
		if (InputManager::getInstance().resetCamPressed)
		{
			lookingInputReturnToDefaultTime = 0.0f;
			lookingInputReturnToDefaultCachedFromInput = lookingInput;
			lookingInputReturnToDefaultCachedToInput = glm::vec2(glm::degrees(std::atan2f(-facingDirection.x, facingDirection.y)), 0.25f);
		}
	}
	else
	{
		// Reset camera!
		float lerpSmoothStepped = PhysicsUtils::smoothStep(0, 1, lookingInputReturnToDefaultTime);
		lookingInput.x = PhysicsUtils::lerpAngleDegrees(lookingInputReturnToDefaultCachedFromInput.x, lookingInputReturnToDefaultCachedToInput.x, lerpSmoothStepped);
		lookingInput.y = PhysicsUtils::lerp(lookingInputReturnToDefaultCachedFromInput.y, lookingInputReturnToDefaultCachedToInput.y, lerpSmoothStepped);

		lookingInputReturnToDefaultTime += 8.0f * MainLoop::getInstance().deltaTime;

		// Keep in this state if still holding cam button
		if (InputManager::getInstance().resetCamPressed)
		{
			lookingInputReturnToDefaultTime = glm::min(lookingInputReturnToDefaultTime, 0.999f);
		}
	}

	//
	// Movement
	//
	glm::vec2 movementVector(InputManager::getInstance().leftStickX, InputManager::getInstance().leftStickY);

	if (glm::length2(movementVector) > 0.001f)
	{
		float movementVectorMagnitude = glm::length(movementVector);
		movementVector = glm::normalize(movementVector) * (movementVectorMagnitude * (1.0f - minMvtVectorMagnitude) + minMvtVectorMagnitude);

		// Change input vector to camera view
		glm::vec3 ThreeDMvtVector =
			movementVector.x * glm::normalize(glm::cross(playerCamera.orientation, playerCamera.up)) +
			movementVector.y * glm::normalize(glm::vec3(playerCamera.orientation.x, 0.0f, playerCamera.orientation.z));
		movementVector.x = ThreeDMvtVector.x;
		movementVector.y = ThreeDMvtVector.z;
		movementVector = PhysicsUtils::clampVector(movementVector, 0.0f, 1.0f);
	}
	else
		movementVector = glm::vec2(0.0f);

#ifdef _DEVELOP
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
	// Change whether using indoor or outdoor camera based off environmental cues (ceiling height)
	//
	const glm::vec3 playerPos = PhysicsUtils::getPosition(getTransform());
	static float cameraTransitionRaw = (float)useIndoorCamera;
	{
		// Check if indoors if ceiling is low
		const physx::PxCapsuleGeometry capsuleGeom(indoorOverlapCheckRadius, indoorOverlapCheckHeight);
		const physx::PxTransform capsulePose(
			PhysicsUtils::toPxVec3(playerPos + glm::vec3(0, indoorOverlapCheckOffY, 0)),
			physx::PxQuat(glm::radians(90.0f), physx::PxVec3(0, 0, 1))
		);

		physx::PxOverlapHit overlapInfo;
		if (PhysicsUtils::overlap(capsuleGeom, capsulePose, overlapInfo))
		{
			useIndoorCamera = true;
		}
		else
		{
			//
			// Just check if perhaps not surrounded by stuff!!
			//
			useIndoorCamera = false;
		}
	}


	//
	// Do cam offset transition depending on if indoors or not
	//
	glm::vec3 camOffset = useIndoorCamera ? playerCamOffsetIndoor : playerCamOffset;

	static glm::vec3 playerCamOffsetCache = playerCamOffset;
	static glm::vec3 playerCamOffsetIndoorCache = playerCamOffsetIndoor;

	const bool cameraInTransition = (cameraTransitionRaw != 0.0f && !useIndoorCamera) || (cameraTransitionRaw != 1.0f && useIndoorCamera);
	if (cameraInTransition)
	{
		cameraTransitionRaw = PhysicsUtils::moveTowards(cameraTransitionRaw, (float)useIndoorCamera, 2.0f * MainLoop::getInstance().deltaTime);
		camOffset = PhysicsUtils::lerp(playerCamOffsetCache, playerCamOffsetIndoorCache, glm::vec3(PhysicsUtils::smoothStep(0, 1, cameraTransitionRaw)));
	}

	//
	// Update playercam pos
	//
	{
		const glm::vec3 cameraTargetPosition = playerPos + glm::vec3(camOffset.x, camOffset.y, 0);
		const float lengthToCamTarget = glm::abs(cameraTargetPosition.y - cameraPosition.y);
		cameraPosition = cameraTargetPosition;
		cameraPosition.y =
			PhysicsUtils::moveTowards(
				cameraPosition.y,
				cameraTargetPosition.y,
				lengthToCamTarget * cameraSpeedMultiplier * MainLoop::getInstance().deltaTime
			);
	}

	// Setup camera orientation
	glm::quat lookingRotation(glm::radians(glm::vec3(lookingInput.y * 85.0f, -lookingInput.x, 0.0f)));
	playerCamera.orientation = lookingRotation * glm::vec3(0, 0, 1);

	// Do raycast to see what the camera distance should be
	{
		float cameraDistance = camOffset.z;

		const bool collideWithGeometry = true;				// Well hey, let's just enable it for now.	// @TEMP: it's getting a bit annoying, so I'll turn off the camera moving in just to see how that's like
		if (collideWithGeometry)
		{
			const float hitDistancePadding = 0.75f;
			physx::PxRaycastBuffer hitInfo;
			if (PhysicsUtils::raycast(
				PhysicsUtils::toPxVec3(((PlayerPhysics*)getPhysicsComponent())->controller->getPosition()) + physx::PxVec3(camOffset.x, camOffset.y, 0.0f),
				PhysicsUtils::toPxVec3(-playerCamera.orientation),
				std::abs(cameraDistance) + hitDistancePadding,
				hitInfo))
			{
				cameraDistance = -hitInfo.block.distance + hitDistancePadding;		// NOTE: must be negative distance since behind model
			}
		}

		//
		// Update max distance member var
		//
		static float scoochBackStartPointCache = currentMaxCamDistance;
		static float scoochBackTransitionRaw = 0.0f;
		if (cameraInTransition || currentMaxCamDistance <= cameraDistance)
		{
			// Reset timer
			currentMaxCamDistance = cameraDistance;
			scoochBackStartPointCache = currentMaxCamDistance;
			scoochBackTransitionRaw = 0.0f;
			maxCamDistanceHoldTimer = maxCamDistanceHoldTime;
		}
		else
		{
			// Scooch back once timer is down
			maxCamDistanceHoldTimer -= MainLoop::getInstance().deltaTime;
			if (isMoving || maxCamDistanceHoldTimer < 0)						// NOTE: Skip over this wait if moving already
			{
				maxCamDistanceHoldTimer = -1;

				scoochBackTransitionRaw =
					PhysicsUtils::moveTowards(
						scoochBackTransitionRaw,
						1.0f,
						maxCamDistanceShiftSpeed * MainLoop::getInstance().deltaTime
					);

				currentMaxCamDistance =
					PhysicsUtils::lerp(scoochBackStartPointCache, cameraDistance, PhysicsUtils::smoothStep(0, 1, scoochBackTransitionRaw));
			}
		}
	}

	//
	// Cache the camera positions
	//
	if (!cameraInTransition)
		if (useIndoorCamera)
		{
			playerCamOffsetCache.z = playerCamOffset.z;
			playerCamOffsetIndoorCache.z = currentMaxCamDistance;
		}
		else
		{
			playerCamOffsetCache.z = currentMaxCamDistance;
			playerCamOffsetIndoorCache.z = playerCamOffsetIndoor.z;
		}

	// Setup camera position based off distance
	const glm::vec3 depthOffset(0, 0, currentMaxCamDistance);
	playerCamera.position = cameraPosition + lookingRotation * depthOffset;

	//
	// Update movement
	//
	targetCharacterLeanValue = 0.0f;
	isMoving = false;

	physx::PxVec3 velocity(0.0f);
	if (((PlayerPhysics*)getPhysicsComponent())->getIsGrounded() && !((PlayerPhysics*)getPhysicsComponent())->getIsSliding())		// @TODO: for some reason sliding and trying to jump doesn't work... why???
	{
		velocity = processGroundedMovement(movementVector);
		jumpCoyoteTimer = jumpCoyoteTime;
	}
	else
	{
		velocity = processAirMovement(movementVector);
		jumpCoyoteTimer -= MainLoop::getInstance().deltaTime;
	}

	// Poll input for the jump (include debounce input)
	jumpInputDebounceTimer -= MainLoop::getInstance().deltaTime;
	static bool prevJumpPressed = false;
	if (!prevJumpPressed && InputManager::getInstance().jumpPressed)
		jumpInputDebounceTimer = jumpInputDebounceTime;
	prevJumpPressed = InputManager::getInstance().jumpPressed;

	// Jump (but not if sliding)
	if (!lockJumping &&
		jumpInputDebounceTimer > 0.0f &&
		jumpCoyoteTimer > 0.0f)
	{
		jumpInputDebounceTimer = -1.0f;
		jumpCoyoteTimer = -1.0f;
		velocity.y = jumpSpeed[GameState::getInstance().playerIsHoldingWater];
		GameState::getInstance().inputStaminaEvent(StaminaEvent::JUMP);
		((PlayerPhysics*)getPhysicsComponent())->setIsGrounded(false);
		((PlayerPhysics*)getPhysicsComponent())->setIsSliding(false);
		triggerAnimationStateReset = true;
	}

	if (isMoving)
		GameState::getInstance().inputStaminaEvent(StaminaEvent::MOVE, MainLoop::getInstance().deltaTime);

	characterLeanValue = PhysicsUtils::lerp(
		characterLeanValue,
		targetCharacterLeanValue,
		leanLerpTime * MainLoop::getInstance().deltaTime
	);

	((PlayerPhysics*)getPhysicsComponent())->velocity = velocity;

	model->localTransform =
		glm::translate(glm::mat4(1.0f), glm::vec3(0, modelOffsetY, 0)) *
		glm::eulerAngleXYZ(0.0f, std::atan2f(facingDirection.x, facingDirection.y), glm::radians(characterLeanValue * 20.0f)) *
		glm::scale(glm::mat4(1.0f), PhysicsUtils::getScale(getTransform()));
}

physx::PxVec3 PlayerCharacter::processGroundedMovement(const glm::vec2& movementVector)
{
	//
	// Update external forces
	//
	float externalStandingAngularVelocityYRadians;
	if (((PlayerPhysics*)getPhysicsComponent())->getStandingOnAngularVelocityY(externalStandingAngularVelocityYRadians))
	{
		// NOTE: bc this is a physics-based value, we need to multiply by the physics delta time, not the regular delta time
		facingDirection = glm::quat(glm::vec3(0.0f, 0.0f, MainLoop::getInstance().physicsDeltaTime * -externalStandingAngularVelocityYRadians)) * glm::vec3(facingDirection, 0.0f);
	}

	//
	// Set Movement Vector
	//
	float movementVectorLength = glm::length2(movementVector);
	if (movementVectorLength > 0.001f)
	{
		isMoving = true;
		//movementVectorLength = 0.5f * movementVectorLength + 0.5f;		// NOTE: having 0.0006f as a movement vector length is too slow

		//
		// Update facing direction
		//
		if (!lockFacingDirection)
		{
			physx::PxVec3 velocityCopy = ((PlayerPhysics*)getPhysicsComponent())->velocity;
			velocityCopy.y = 0.0f;
			float flatVelocityMagnitude = velocityCopy.magnitude();

			// Skid stop behavior:
			float mvtDotFacing = glm::dot(movementVector, facingDirection);
			if (flatVelocityMagnitude > immediateTurningRequiredSpeed && mvtDotFacing < -0.707106781f)
			{
				facingDirection = movementVector;
				currentRunSpeed = -currentRunSpeed; // NOTE: this makes more sense especially when you land on the ground and inherit a butt ton of velocity   //0;
			}
			// Immediate facing direction when moving slowly:
			else if (flatVelocityMagnitude <= immediateTurningRequiredSpeed)
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

				float facingTurnSpeedCalculated = glm::clamp(-groundedFacingTurnSpeed / (groundRunSpeedCantTurn - groundRunSpeed) * (flatVelocityMagnitude - groundRunSpeed) + groundedFacingTurnSpeed, 0.0f, groundedFacingTurnSpeed);
				//std::cout << "FACTSC: \t\t\t\t" << facingTurnSpeedCalculated << std::endl;
				float newFacingDirectionAngle = glm::radians(PhysicsUtils::moveTowardsAngle(facingDirectionAngle, targetDirectionAngle, facingTurnSpeedCalculated * MainLoop::getInstance().deltaTime));

				facingDirection = glm::vec2(std::sinf(newFacingDirectionAngle), std::cosf(newFacingDirectionAngle));

				//
				// Calculate lean amount (use targetDirectionAngle bc of lack of deltaTime mess)
				//
				float deltaFacingDirectionAngle = targetDirectionAngle - facingDirectionAngle;

				if (deltaFacingDirectionAngle < -180.0f)			deltaFacingDirectionAngle += 360.0f;
				else if (deltaFacingDirectionAngle > 180.0f)		deltaFacingDirectionAngle -= 360.0f;

				const float deadZoneDeltaAngle = 0.1f;
				if (std::abs(deltaFacingDirectionAngle) > deadZoneDeltaAngle)
					targetCharacterLeanValue = std::clamp(-deltaFacingDirectionAngle * leanMultiplier, -1.0f, 1.0f);
				else
					targetCharacterLeanValue = 0.0f;
			}
		}
	}
	else
		movementVectorLength = 0.0f;

	//
	// Update Running Speed
	//
	float targetRunningSpeed = movementVectorLength * groundRunSpeed;

	//std::cout << (currentRunSpeed >= 0.0f && currentRunSpeed < targetRunningSpeed ? "00000\t\t" : "11111\t\t") << currentRunSpeed;

	currentRunSpeed = PhysicsUtils::moveTowards(
		currentRunSpeed,
		targetRunningSpeed,
		(currentRunSpeed >= 0.0f && currentRunSpeed < targetRunningSpeed ? groundAcceleration : groundDecceleration) *		// NOTE: when doing the turnaround and currentRunSpeed = -currentRunSpeed, the target running speed is technically > currentRunSpeed, so you get the acceleration coefficient, however, we want the deceleration one, bc the player is scooting backwards. It should just be that way.  -Timo
		MainLoop::getInstance().deltaTime
	);
	//std::cout << "\t\t" << currentRunSpeed << std::endl;

	//
	// Apply the maths onto the actual velocity vector now!
	//
	glm::vec3 velocity = glm::vec3(facingDirection.x, 0, facingDirection.y) * currentRunSpeed;

	if (((PlayerPhysics*)getPhysicsComponent())->getIsSliding())
	{
		// Cut off movement towards uphill if supposed to be sliding
		const glm::vec3 normal = ((PlayerPhysics*)getPhysicsComponent())->getGroundedNormal();
		const glm::vec3 flatNormal = glm::normalize(glm::vec3(normal.x, 0, normal.z));
		if (glm::dot(flatNormal, glm::vec3(facingDirection.x, 0, facingDirection.y)) < 0.0f)	// If going against the direction it's supposed to slide
		{
			glm::vec3 TwoDNormal = glm::normalize(glm::vec3(normal.z, 0.0f, -normal.x));		// Flip positions to get the 90 degree right vector
			velocity = glm::dot(TwoDNormal, velocity) * TwoDNormal;								// Project the velocity vector onto the 90 degree flat vector;
		}
	}
	else
	{
		// Add on a grounded rotation based on the ground you're standing on (!isSliding)
		glm::quat slopeRotation = glm::rotation(
			glm::vec3(0, 1, 0),
			((PlayerPhysics*)getPhysicsComponent())->getGroundedNormal()
		);
		velocity = slopeRotation * velocity;
	}

	//
	// Fetch in physics y (overwriting)
	// (HOWEVER, not if going down a slope's -y is lower than the reported y (ignore when going up/jumping))
	//
	if (velocity.y < 0.0f)
		velocity.y = std::min(((PlayerPhysics*)getPhysicsComponent())->velocity.y, velocity.y);
	else
		velocity.y = ((PlayerPhysics*)getPhysicsComponent())->velocity.y;

	return physx::PxVec3(velocity.x, velocity.y, velocity.z);
}

physx::PxVec3 PlayerCharacter::processAirMovement(const glm::vec2& movementVector)
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

		facingDirectionAngle = glm::radians(PhysicsUtils::moveTowardsAngle(facingDirectionAngle, targetDirectionAngle, airBourneFacingTurnSpeed * MainLoop::getInstance().deltaTime));

		facingDirection = glm::vec2(std::sinf(facingDirectionAngle), std::cosf(facingDirectionAngle));
	}

	//
	// Manipulate the flat velocity
	//
	physx::PxVec3 currentVelocity = ((PlayerPhysics*)getPhysicsComponent())->velocity;
	glm::vec2 currentFlatVelocity = glm::vec2(currentVelocity.x, currentVelocity.z);
	glm::vec2 targetFlatVelocity = movementVector * groundRunSpeed;

	currentFlatVelocity = PhysicsUtils::moveTowardsVec2(currentFlatVelocity, targetFlatVelocity, airAcceleration * MainLoop::getInstance().deltaTime);
	currentVelocity.x = currentFlatVelocity.x;
	currentVelocity.z = currentFlatVelocity.y;

	return currentVelocity;
}

void PlayerCharacter::processActions()
{
	if (GameState::getInstance().playerIsHoldingWater &&
		InputManager::getInstance().useItemPressed)
	{
		// Drink the water
		GameState::getInstance().playerIsHoldingWater = false;
		GameState::getInstance().currentPlayerStaminaAmount =
			(float)GameState::getInstance().maxPlayerStaminaAmount;

		triggerDrinkWaterAnimation = true;
	}
}

float hairWeightMult = 10.0f;				// @Debug
float speedAnimRunningMult = 1.3f;			// @Debug
float speedAnimRunningFloor = 0.525f;		// @Debug
void PlayerCharacter::processAnimation()
{
	constexpr int IDLE_ANIM = 0;
	constexpr int WALKING_ANIM = 1;
	constexpr int RUNNING_ANIM = 2;
	constexpr int JUMP_ANIM = 3;
	constexpr int LAND_ANIM = 5;
	constexpr int DRAW_WATER_ANIM = 9;
	constexpr int DRINK_WATER_ANIM = 10;
	constexpr int SHEATH_BOTTLE_ANIM = 11;

	//
	// Process movement into animationstates
	//
	if (animationState == 0)
	{
		if (isMoving)
			// Move
			animationState = 1;
		if (!((PlayerPhysics*)getPhysicsComponent())->getIsGrounded())
			// Jump
			animationState = 2;
	}
	else if (animationState == 1)
	{
		if (!isMoving)
			// Idle
			animationState = 0;
		if (!((PlayerPhysics*)getPhysicsComponent())->getIsGrounded())
			// Jump
			animationState = 2;
	}
	else if (animationState == 2)
	{
		if (((PlayerPhysics*)getPhysicsComponent())->getIsGrounded() && !((PlayerPhysics*)getPhysicsComponent())->getIsSliding())
			// Landing
			animationState = 3;
	}
	else if (animationState == 3)
	{
		if (!((PlayerPhysics*)getPhysicsComponent())->getIsGrounded())
			// Jump
			animationState = 2;
		else
		{
			// Wait until land animation is finished
			if (animator.isAnimationFinished(LAND_ANIM, MainLoop::getInstance().deltaTime) ||
				animator.isAnimationFinished(LAND_ANIM + 1, MainLoop::getInstance().deltaTime))
			{
				// Standing
				animationState = 0;
				//std::cout << "Switched to standing" << std::endl;
			}
		}
	}
	else if (animationState == 4)
	{
		// Lock movement
		((PlayerPhysics*)getPhysicsComponent())->lockVelocity(false);
		lockFacingDirection = true;
		lockJumping = true;

		// Wait until draw water animation is finished
		if (animator.isAnimationFinished(DRAW_WATER_ANIM, MainLoop::getInstance().deltaTime))
		{
			// Pick up bottle
			animationState = 6;
		}
	}
	else if (animationState == 5)
	{
		// Lock movement
		((PlayerPhysics*)getPhysicsComponent())->lockVelocity(false);
		lockFacingDirection = true;
		lockJumping = true;

		// Wait until drink-water animation is finished
		if (animator.isAnimationFinished(DRINK_WATER_ANIM, MainLoop::getInstance().deltaTime))
		{
			// Pick up bottle
			animationState = 6;
		}
	}
	else if (animationState == 6)
	{
		// Lock movement
		((PlayerPhysics*)getPhysicsComponent())->lockVelocity(false);
		lockFacingDirection = true;
		lockJumping = true;

		// Wait until pick-up-bottle animation is finished
		if (animator.isAnimationFinished(SHEATH_BOTTLE_ANIM, MainLoop::getInstance().deltaTime))
		{
			// Standing
			animationState = 0;
			lockFacingDirection = false;
			lockJumping = false;
		}
	}

	if (Messages::getInstance().checkForMessage("PlayerCollectWater"))
	{
		animationState = 4;
	}

	if (triggerDrinkWaterAnimation)
	{
		triggerDrinkWaterAnimation = false;
		animationState = 5;
	}

	//
	// Update Animation State
	//
	if (triggerAnimationStateReset ||
		prevAnimState != animationState)
	{
		triggerAnimationStateReset = false;		// Reset flag

		switch (animationState)
		{
		case 0:
			// Idle
			animator.playAnimation(IDLE_ANIM + (unsigned int)isMoving, 6.0f, true, true);
			break;

		case 1:
			// Move
			animator.playBlendTree(
				{
					{ (size_t)WALKING_ANIM, minMvtVectorMagnitude, "walkRunBlendVar" },
					{ (size_t)RUNNING_ANIM, groundRunSpeed }
				},
				6.0f, true
			);
			break;

		case 2:
			// Jump
			animator.playAnimation(JUMP_ANIM + (int)isMoving, 0.0f, false, true);
			break;

		case 3:
			// Land
			animator.playAnimation(LAND_ANIM + (int)isMoving, 0.0f, false, true);
			break;

		case 4:
			// Draw water
			animator.playAnimation(DRAW_WATER_ANIM, 1.0f, false, true);
			bottleModel->setDepthPriorityOfMeshesWithMaterial("Water", (GameState::getInstance().playerIsHoldingWater ? 1.0f : -1.0f));		// @HACK: shouldn't be using transparency ordering hax like this
			break;

		case 5:
			// Drink water
			animator.playAnimation(DRINK_WATER_ANIM, 1.0f, false, true);
			bottleModel->setDepthPriorityOfMeshesWithMaterial("Water", (GameState::getInstance().playerIsHoldingWater ? 1.0f : -1.0f));		// @HACK: shouldn't be using transparency ordering hax like this
			break;

		case 6:
			// Pick up bottle
			animator.playAnimation(SHEATH_BOTTLE_ANIM, 7.5f, false, true);
			break;

		default:
			std::cout << "Animation State " << animationState << " not recognized." << std::endl;
			break;
		}
	}

	prevAnimState = animationState;

	//
	// Mesh Skinning
	//
	if (animationState == WALKING_ANIM)
	{
		physx::PxVec3 velo = ((PlayerPhysics*)getPhysicsComponent())->velocity;
		velo.y = 0.0f;
		float flatSpeed = velo.magnitude();

		animator.setBlendTreeVariable("walkRunBlendVar", glm::clamp(REMAP(flatSpeed, 0.4f, groundRunSpeed, 0.0f, 1.0f), 0.0f, 1.0f));

		animator.animationSpeed = animationSpeed * flatSpeed * speedAnimRunningMult + speedAnimRunningFloor;
	}
	else
		animator.animationSpeed = animationSpeed;
	animator.updateAnimation(MainLoop::getInstance().deltaTime);		// Correction: this adds more than 10ms consistently

	//
	// @TODO: Do IK (Forward and Backward Reaching Inverse Kinematics for a heuristic approach)
	//
#ifdef _DEVELOP
	if (MainLoop::getInstance().playMode)
#endif
	{
		const glm::mat4 globalTransform = getTransform() * model->localTransform;

		if (rightSideburn.isFirstTime || leftSideburn.isFirstTime || backAttachment.isFirstTime)
		{
			//
			// Setup Rope simulations
			//
			std::vector<glm::vec3> rightSideburnPoints;
			const glm::vec4 neutralPosition(0, 0, 0, 1);
			rightSideburnPoints.push_back(globalTransform * animator.getBoneTransformation("Hair Sideburn1.R").globalTransformation * neutralPosition);
			rightSideburnPoints.push_back(globalTransform * animator.getBoneTransformation("Hair Sideburn2.R").globalTransformation * neutralPosition);
			rightSideburnPoints.push_back(globalTransform * animator.getBoneTransformation("Hair Sideburn3.R").globalTransformation * neutralPosition);
			rightSideburnPoints.push_back(globalTransform * animator.getBoneTransformation("Hair Sideburn4.R").globalTransformation * neutralPosition);
			rightSideburn.initializePoints(rightSideburnPoints);

			std::vector<glm::vec3> leftSideburnPoints;
			leftSideburnPoints.push_back(globalTransform * animator.getBoneTransformation("Hair Sideburn1.L").globalTransformation * neutralPosition);
			leftSideburnPoints.push_back(globalTransform * animator.getBoneTransformation("Hair Sideburn2.L").globalTransformation * neutralPosition);
			leftSideburnPoints.push_back(globalTransform * animator.getBoneTransformation("Hair Sideburn3.L").globalTransformation * neutralPosition);
			leftSideburnPoints.push_back(globalTransform * animator.getBoneTransformation("Hair Sideburn4.L").globalTransformation * neutralPosition);
			leftSideburn.initializePoints(leftSideburnPoints);

			std::vector<glm::vec3> backAttachmentPoints;
			backAttachmentPoints.push_back(globalTransform * animator.getBoneTransformation("Back Attachment").globalTransformation * neutralPosition);
			backAttachmentPoints.push_back(globalTransform * animator.getBoneTransformation("Back Attachment").globalTransformation * (neutralPosition + glm::vec4(0, -10, 0, 0)));
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
			leftSideburn.setPointPosition(0, 0.05f, globalTransform * animator.getBoneTransformation("Hair Sideburn1.L").globalTransformation * neutralPosition);
			rightSideburn.setPointPosition(0, 0.05f, globalTransform * animator.getBoneTransformation("Hair Sideburn1.R").globalTransformation * neutralPosition);
			backAttachment.setPointPosition(0, 0.025f, globalTransform * animator.getBoneTransformation("Back Attachment").globalTransformation * neutralPosition);

			leftSideburn.simulateRope(hairWeightMult);
			rightSideburn.simulateRope(hairWeightMult);
			backAttachment.simulateRope(hairWeightMult *10.0f);

			//
			// Do ik calculations for sideburns
			//
			const glm::mat4 inverseRenderTransform = glm::inverse(globalTransform);

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

		//
		// Calculate the bottle transformation matrix
		// @@@TODO: fix the baseObject->getTransform() areas, bc the transformation hierarchy isn't established yet.
		if (animationState == 4 || animationState == 5 || animationState == 6)
			bottleModel->localTransform = model->localTransform * animator.getBoneTransformation("Hand Attachment").globalTransformation * bottleHandModelMatrix;
		else
			bottleModel->localTransform = model->localTransform * animator.getBoneTransformation("Back Attachment").globalTransformation * bottleModelMatrix;
	}
}

void PlayerCharacter::preRenderUpdate()
{
	processMovement();
	processActions();
	processAnimation();

	//
	// Process ditherAlpha
	//
	const float lengthFromCamera = glm::length(MainLoop::getInstance().camera.position - PhysicsUtils::getPosition(getTransform()));
	const float ditherAlpha = (lengthFromCamera - 1.0f) * 0.2f;
	for (auto it = materials.begin(); it != materials.end(); it++)
	{
		it->second->ditherAlpha = ditherAlpha;
	}
	for (auto it = bottleModelMaterials.begin(); it != bottleModelMaterials.end(); it++)
	{
		it->second->ditherAlpha = ditherAlpha;
	}
}

#ifdef _DEVELOP
bool showPlayerIndoorDetectionCapsuleOverlap = false;

void PlayerCharacter::imguiPropertyPanel()
{
	ImGui::DragFloat3("Controller up direction", &((PlayerPhysics*)getPhysicsComponent())->tempUp[0]);			// @NOTE: this doesn't seem necessary with the gameplay... but may be in the future????
	((PlayerPhysics*)getPhysicsComponent())->controller->setUpDirection(((PlayerPhysics*)getPhysicsComponent())->tempUp.getNormalized());

	ImGui::Separator();
	ImGui::Text("Virtual Camera");
	ImGui::DragFloat("Camera Speed Multiplier", &cameraSpeedMultiplier);
	ImGui::DragFloat3("VirtualCamPosition", &playerCamOffset[0]);
	ImGui::DragFloat3("VirtualCamPosition (Indoor)", &playerCamOffsetIndoor[0]);
	ImGui::DragFloat2("Looking Input", &lookingInput[0]);
	ImGui::DragFloat2("Looking Sensitivity", &lookingSensitivity[0]);

	ImGui::Separator();
	ImGui::Text("Movement Properties");
	ImGui::DragFloat("Running Acceleration", &groundAcceleration, 0.1f);
	ImGui::DragFloat("Running Decceleration", &groundDecceleration, 0.1f);
	ImGui::DragFloat("Running Acceleration (Air)", &airAcceleration, 0.1f);
	ImGui::DragFloat("Running Speed", &groundRunSpeed, 0.1f);
	ImGui::DragFloat2("Jump Speed", &jumpSpeed[0], 0.1f);
	ImGui::Text(("Facing Direction: (" + std::to_string(facingDirection.x) + ", " + std::to_string(facingDirection.y) + ")").c_str());
	ImGui::DragFloat("Leaning Lerp Time", &leanLerpTime);
	ImGui::DragFloat("Lean Multiplier", &leanMultiplier, 0.05f);
	ImGui::DragFloat("Facing Movement Speed", &groundedFacingTurnSpeed, 0.1f);
	ImGui::DragFloat("Facing Movement Speed (Air)", &airBourneFacingTurnSpeed, 0.1f);

	ImGui::Separator();
	ImGui::DragFloat("Model Offset Y", &modelOffsetY, 0.05f);
	ImGui::DragFloat("Model Animation Speed", &animationSpeed);
	ImGui::DragFloat("Model Run/Walk mult", &speedAnimRunningMult);
	ImGui::DragFloat("Model Run/Walk floor", &speedAnimRunningFloor);

	physx::PxVec3 velo = ((PlayerPhysics*)getPhysicsComponent())->velocity;
	velo.y = 0.0f;
	float flatSpeed = velo.magnitude();
	ImGui::Text(("Speed: " + std::to_string(flatSpeed)).c_str());
	ImGui::Text(("Evaluated: " + std::to_string(flatSpeed * speedAnimRunningMult + speedAnimRunningFloor)).c_str());
	ImGui::Text(("MIN: " + std::to_string(minMvtVectorMagnitude * speedAnimRunningMult + speedAnimRunningFloor)).c_str());
	ImGui::Text(("MAX: " + std::to_string(groundRunSpeed * speedAnimRunningMult + speedAnimRunningFloor)).c_str());

	ImGui::Separator();
	ImGui::ColorPicker3("Body Zelly Color", &((ZellyMaterial*)materials["Body"])->getColor().x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
	ImGui::ColorPicker3("Hair Zelly Color", &((ZellyMaterial*)materials["Hair"])->getColor().x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

	ImGui::Separator();
	ImGui::DragFloat("Hair Weight", &hairWeightMult);

	ImGui::Separator();
	ImGui::Checkbox("Indoor check show visual", &showPlayerIndoorDetectionCapsuleOverlap);
	ImGui::DragFloat("Indoor check radius", &indoorOverlapCheckRadius);
	ImGui::DragFloat("Indoor check height", &indoorOverlapCheckHeight);
	ImGui::DragFloat("Indoor check offset y", &indoorOverlapCheckOffY);

	
	
	// @Tune: these will mess up the matrices bc of how the imguiTransformMatrixProps() func works
	//ImGui::Separator();
	//ImGui::Text("Bottle Model Matrix");
	//PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(((PlayerRender*)baseObject->getRenderComponent())->bottleModelMatrix));
	//ImGui::Text("Bottle Hand Model Matrix");
	//PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(((PlayerRender*)baseObject->getRenderComponent())->bottleHandModelMatrix));
}

void PlayerCharacter::imguiRender()
{
	if (showPlayerIndoorDetectionCapsuleOverlap)
	{
		PhysicsUtils::imguiRenderCapsuleCollider(
			glm::translate(getTransform(), glm::vec3(0, indoorOverlapCheckOffY, 0)) *
				glm::toMat4(glm::quat({ 0.0f, 0.0f, glm::radians(90.0f) })),
			indoorOverlapCheckRadius,
			indoorOverlapCheckHeight
		);
	}
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

void RopeSimulation::setPointPosition(size_t index, float trickleRate, const glm::vec3& position)
{
	glm::vec3 deltaMovement = position - points[index];
	points[index] = position;

	// NOTE: this is for alleviating glitchyness when point moves too much.
	// This code propogates the movement from this new set operation to the other points of the rope
	for (size_t i = 0; i < prevPoints.size(); i++)
	{
		prevPoints[i] += deltaMovement * glm::pow(trickleRate, (float)(i + 1));		// NOTE: I tried doing size-i, bc I thought that was more correct, but no, just doing i+1 for the exponent is correct -Timo 01-17-2022
	}
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
