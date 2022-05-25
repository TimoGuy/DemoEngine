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


// -----------------------------------------------------------------------------------------------------------------------------------------------------------
// ROPESIMULATION
// -----------------------------------------------------------------------------------------------------------------------------------------------------------
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


// -----------------------------------------------------------------------------------------------------------------------------------------------------------
// PLAYERPHYSICS
// -----------------------------------------------------------------------------------------------------------------------------------------------------------
float playerCapsuleControllerRadius = 1.0f;
PlayerPhysics::PlayerPhysics(BaseObject* bo) : PhysicsComponent(bo)
{
	controller =
		PhysicsUtils::createCapsuleController(
			MainLoop::getInstance().physicsControllerManager,
			MainLoop::getInstance().defaultPhysicsMaterial,
			physx::PxExtendedVec3(0.0f, 100.0f, 0.0f),
			playerCapsuleControllerRadius,
			4.5f,
			this,		// PxUserControllerHitReport
			this		// PxBehaviorCallbackWhatevs
		);

	GameState::getInstance().playerActorPointer = controller->getActor();
	body = controller->getActor();

	physx::PxFilterData filterData;
	filterData.word0 = (physx::PxU32)PhysicsUtils::Word0Tags::ENTITY;

	std::vector<physx::PxShape*> shapes;
	shapes.resize(body->getNbShapes());
	int numShapes = body->getShapes(&shapes[0], shapes.size());
	for (size_t i = 0; i < numShapes; i++)
		shapes[i]->setQueryFilterData(filterData);
}

PlayerPhysics::~PlayerPhysics()
{
	controller->release();
}

void PlayerPhysics::lockVelocity(bool yAlso)
{
	if (yAlso)
		velocity = physx::PxVec3(0);
	else
	{
		velocity.x = 0;
		velocity.z = 0;
	}
}


#ifdef _DEVELOP
bool playerPhysicsOverrideVelocityInheritanceBcOfImguiInterference = true;		// NOTE: just in case first frame may get messed up, this flag is set to true at first.
#endif


void PlayerPhysics::physicsUpdate()
{
	//
	// Add gravity (or sliding gravity if sliding)
	//
	velocity.y -= 4.9f * MainLoop::getInstance().physicsDeltaTime;

	physx::PxVec3 cookedVelocity = velocity;

	// Add force to go down stairs
	if (isGrounded && !isSliding)
		cookedVelocity.y = -controller->getStepOffset();

	//
	// Do the deed... move();
	//
	physx::PxControllerCollisionFlags collisionFlags = controller->move(cookedVelocity, 0.01f, MainLoop::getInstance().physicsDeltaTime, NULL, NULL);
	bool prevIsSliding = isSliding;
	isGrounded = false;
	isSliding = false;
	isSlidingCeiling = false;
	isSandwiching = false;
	hasValidStandingOnAngularVelocityY = false;

	// Check if collision on top and bottom
	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN &&
		collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_UP)
	{
		// Move away at the average of the two normals
		glm::vec3 avgHitNormal = (groundHitNormal + ceilingHitNormal) / 2.0f;
		avgHitNormal.y = 0.0f;
		avgHitNormal = glm::normalize(avgHitNormal);

		controller->move(
			PhysicsUtils::toPxVec3(avgHitNormal * 0.1f),
			0.01f,
			MainLoop::getInstance().physicsDeltaTime,
			NULL,
			NULL
		);
	}

	// Collision on bottom
	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN)
	{
		isGrounded = true;
		bool isFlatGround = (glm::dot(groundHitNormal, glm::vec3(0, 1, 0)) > 0.69465837f);

		// Try to retrieve grounded info (don't assign to isFlatGround if currently sliding)
		physx::PxRaycastBuffer hitInfo;
		constexpr float padding = 0.05f;
		if (PhysicsUtils::raycast(
			PhysicsUtils::toPxVec3(controller->getFootPosition()) + physx::PxVec3(0, padding, 0),
			physx::PxVec3(0, -1, 0),
			controller->getStepOffset() + padding + padding,
			hitInfo)
			&& !prevIsSliding)
			isFlatGround |= (glm::dot(PhysicsUtils::toGLMVec3(hitInfo.block.normal), glm::vec3(0, 1, 0)) > 0.69465837f);

		if (isFlatGround)
		{
			velocity.y = 0.0f;		// Remove gravity

			if (hitInfo.hasBlock && (hitInfo.block.actor->getType() & physx::PxActorType::eRIGID_DYNAMIC))		// @TODO: @REFACTOR: attach the actor via the onshapecollide() callback instead of a downwards raycast (only use as fallback)
			{
				physx::PxRigidDynamic* body = (physx::PxRigidDynamic*)hitInfo.block.actor;
				standingOnAngularVelocityYRadians = body->getAngularVelocity().y;
				hasValidStandingOnAngularVelocityY = true;
			}
		}
		else if (velocity.y < 0.0f)
		{
			isSliding = true;		// Slide down!

			// Slide down normal going downhill (NOTE: This algorithm is different from the slide up normal uphill counterpart. This is bc the way the capsule controller treats these types of collisions is different. This version pushes the controller horizontally away from the wall, the uphill version redirects the upwards velocity along the normal of the wall  -Timo)
			float fallVelocity = velocity.y - offsetFootMovedReconstructed.y;
			const glm::vec3 upXnormal = glm::cross(glm::vec3(0, 1, 0), groundHitNormal);
			const glm::vec3 uxnXnormal = glm::normalize(glm::cross(upXnormal, groundHitNormal));
			const glm::vec2 slidingVectorXZ = glm::vec2(uxnXnormal.x, uxnXnormal.z) * fallVelocity / uxnXnormal.y;

			glm::vec2 pushOffVelocity(0.0f);
			glm::vec2 flatVelocity(velocity.x, velocity.z);
			if (glm::length2(flatVelocity) > 0.001f)
				pushOffVelocity = glm::min(glm::dot(glm::normalize(glm::vec2(groundHitNormal.x, groundHitNormal.z)), glm::normalize(flatVelocity)), 0.0f) * flatVelocity;

			physx::PxVec3 offsetVector =
				physx::PxVec3(
					slidingVectorXZ.x,
					fallVelocity,
					slidingVectorXZ.y
				);
			controller->move(
				offsetVector,
				0.01f,
				MainLoop::getInstance().physicsDeltaTime,
				NULL,
				NULL
			);
		}
	}

	// Collision on sides
	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_SIDES)
	{
		//std::cout << "\tSide Collision";
	}

	// Collision on head
	if (velocity.y > 0.0f && collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_UP)
	{
		if (glm::dot(ceilingHitNormal, glm::vec3(0, -1, 0)) > 0.69465837f)
		{
			velocity.y = 0.0f;		// Hit your head on the ceiling
		}
		else if (velocity.y > 0.0f)
		{
			isSlidingCeiling = true;

			// Slide up normal going uphill (https://www.youtube.com/watch?v=GI5LAbP5slE)
			float riseVelocity = velocity.y - offsetHeadMovedReconstructed.y;
			const glm::vec3 slopeDirection = glm::vec3(0, -1, 0) - ceilingHitNormal * glm::dot(glm::vec3(0, -1, 0), ceilingHitNormal);
			const glm::vec3 moveDirection = -riseVelocity * slopeDirection;

			physx::PxVec3 offsetVector = PhysicsUtils::toPxVec3(moveDirection);
			controller->move(
				offsetVector,
				0.01f,
				MainLoop::getInstance().physicsDeltaTime,
				NULL,
				NULL
			);
#ifdef _DEVELOP
			playerPhysicsOverrideVelocityInheritanceBcOfImguiInterference = true;		// NOTE: Prevent single-tick bounce off when ceiling is right above you. Since isGrounded's state changes at the same time you do this, then you fly away real quick  -Timo
#endif
		}
	}

	//
	// Apply transform
	//
	physx::PxExtendedVec3 pos = controller->getPosition();
	glm::mat4 trans = baseObject->getTransform();
	trans[3] = glm::vec4(pos.x, pos.y, pos.z, 1.0f);
	baseObject->INTERNALsubmitPhysicsCalculation(trans);

	//
	// Set the velocity when leaving or landing the ground
	//
#ifdef _DEVELOP
	if (playerPhysicsOverrideVelocityInheritanceBcOfImguiInterference)		// BLOCKING FLAG
		playerPhysicsOverrideVelocityInheritanceBcOfImguiInterference = false;
	else
#endif
		if (prevIsGrounded != isGrounded)
		{
			// Set the velocity!
			velocity = PhysicsUtils::toPxVec3(pos) - prevPositionWhileGrounded;

			if (isGrounded)
			{
				// Landing on the ground
				glm::vec2 flatVelocity(velocity.x, velocity.z);
				float newRunSpeed = glm::length(flatVelocity);

				if (newRunSpeed > 0.01f)
				{
					glm::vec2 idealFacingDirection = glm::normalize(flatVelocity);
					float facingDotIdeal = glm::dot(((PlayerCharacter*)baseObject)->facingDirection, idealFacingDirection);
					if (facingDotIdeal < 0.0f)
					{
						idealFacingDirection *= -1.0f;
						newRunSpeed *= -1.0f;
					}

					((PlayerCharacter*)baseObject)->facingDirection = idealFacingDirection;		// TODO: figger out turning speed problems!!! (MAYBE: just have there be an exponential function on turn speed. If moving a certain speed, make turn speed move towards 0 like asymptote)
					((PlayerCharacter*)baseObject)->currentRunSpeed = newRunSpeed;
				}
			}
			else
				// Leaving the ground
				velocity.y = glm::max(velocity.y, 0.0f);
		}
	prevPositionWhileGrounded = PhysicsUtils::toPxVec3(pos);		// Use this to create the velocity
	prevIsGrounded = isGrounded;

	// @NOTE: Make this not show up in the release game!!!! @DEBUG only... but it's still kinda glitchy so we need it.
	//#ifdef _DEVELOP
		// Set a save point for the player if press pause!
	static physx::PxExtendedVec3 savedPlayerResetPoint{ 0, 4, 0 };
	if (InputManager::getInstance().pausePressed)
	{
		savedPlayerResetPoint = controller->getFootPosition();
		std::cout << "::PLAYER:: set reset point to { " << savedPlayerResetPoint.x << ", " << savedPlayerResetPoint.y << ", " << savedPlayerResetPoint.z << " }" << std::endl;
	}

	// Reset position of player to 0, 0, 0!
	if (InputManager::getInstance().inventoryPressed)
	{
		controller->setFootPosition(savedPlayerResetPoint);
		velocity = { 0, 0, 0 };
		((PlayerCharacter*)baseObject)->currentRunSpeed = 0.0f;
	}
	//#endif
}

void PlayerPhysics::propagateNewTransform(const glm::mat4& newTransform)
{
	glm::vec3 pos = PhysicsUtils::getPosition(newTransform);
	controller->setPosition(physx::PxExtendedVec3(pos.x, pos.y, pos.z));

#ifdef _DEVELOP
	playerPhysicsOverrideVelocityInheritanceBcOfImguiInterference = true;
#endif
}

physx::PxTransform PlayerPhysics::getGlobalPose()
{
	physx::PxExtendedVec3 pos = controller->getPosition();
	return PhysicsUtils::createTransform(glm::vec3(pos.x, pos.y, pos.z));
}

void PlayerPhysics::onShapeHit(const physx::PxControllerShapeHit& hit)
{
	if (hit.worldNormal.y >= 0.0f)
	{
		groundHitNormal = glm::vec3(hit.worldNormal.x, hit.worldNormal.y, hit.worldNormal.z);
		offsetFootMovedReconstructed =
			PhysicsUtils::toPxExtendedVec3(
				(hit.worldPos
					+ PhysicsUtils::toPxExtendedVec3(hit.worldNormal * playerCapsuleControllerRadius)
					+ physx::PxExtendedVec3(0, -playerCapsuleControllerRadius, 0))
				- controller->getFootPosition()
			);		// NOTE: this is probably getting truncated, the data types. for some reason pxextendedvec3-pxextendedvec3=pxvec3???
	}
	else
	{
		ceilingHitNormal = glm::vec3(hit.worldNormal.x, hit.worldNormal.y, hit.worldNormal.z);
		offsetHeadMovedReconstructed =
			PhysicsUtils::toPxExtendedVec3(
				(hit.worldPos
					+ PhysicsUtils::toPxExtendedVec3(hit.worldNormal * playerCapsuleControllerRadius)
					+ physx::PxExtendedVec3(0, -controller->getHeight() - playerCapsuleControllerRadius, 0))
				- controller->getFootPosition()
			);		// NOTE: this is probably getting truncated, the data types. for some reason pxextendedvec3-pxextendedvec3=pxvec3???
	}
}

void PlayerPhysics::onControllerHit(const physx::PxControllersHit& hit) { PX_UNUSED(hit); }
void PlayerPhysics::onObstacleHit(const physx::PxControllerObstacleHit& hit) { PX_UNUSED(hit); }

// https://github.com/NVIDIAGameWorks/PhysX-3.4/blob/master/PhysX_3.4/Samples/SampleBridges/SampleBridgesCCT.cpp
physx::PxControllerBehaviorFlags PlayerPhysics::getBehaviorFlags(const physx::PxShape& shape, const physx::PxActor& actor)
{
	return physx::PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;		// NOTE: the physx-provided sliding feature doesn't work so well eh.
}

physx::PxControllerBehaviorFlags PlayerPhysics::getBehaviorFlags(const physx::PxController&)
{
	return physx::PxControllerBehaviorFlags(0);
}

physx::PxControllerBehaviorFlags PlayerPhysics::getBehaviorFlags(const physx::PxObstacle&)
{
	return physx::PxControllerBehaviorFlags(0);
}


// -----------------------------------------------------------------------------------------------------------------------------------------------------------
// PLAYERCHARACTER
// -----------------------------------------------------------------------------------------------------------------------------------------------------------
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
		bottleModelMaterials["Water"] = &BottledWaterBobbingMaterial::getInstance();
		bottleModelMaterials["StaminaMeter"] = &StaminaMeterMaterial::getInstance();

		bottleModel->setMaterials(bottleModelMaterials);
		bottleModel->setDepthPriorityOfMeshesWithMaterial("SeeThruRubber", 0.0f);
		bottleModel->setDepthPriorityOfMeshesWithMaterial("Water", 1.0f);		// @HACK: @HAX: Make Water render before SeeThruRubber. This is of course NG and I should never ship a game with this technique in it lol  -Timo
		
		// @NOTE: Hopefully this doesn't segfault lol  -Timo
		std::vector<Mesh>& bottleModelMeshes = bottleModel->getMeshes();
		for (size_t i = 0; i < bottleModelMeshes.size(); i++)
		{
			if (bottleModelMeshes[i].getMaterialName() == "Water")
			{
				bottleModelWaterMesh = &bottleModelMeshes[i];
				break;
			}
		}
	}
}

constexpr float minMvtVectorMagnitude = 0.35f;
void PlayerCharacter::processMovement()
{
	//
	// Get looking input
	//
	useFollowCamera = true;
	if (lookingInputReturnToDefaultTime >= 1.0f)
	{
		lookingInput += glm::vec2(InputManager::getInstance().rightStickX, InputManager::getInstance().rightStickY) * lookingSensitivity;
		lookingInput.x = fmodf(lookingInput.x, 360.0f);
		lookingInput.y = std::clamp(lookingInput.y, -1.0f, 1.0f);

		if (lookingInput.x < 0.0f)
			lookingInput.x += 360.0f;

		if (glm::abs(InputManager::getInstance().rightStickX) > 0.001f)
			useFollowCamera = false;

		// Reset to default if reset camera button pressed
		if (InputManager::getInstance().on_resetCamPressed)
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
		useFollowCamera = false;

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
	// Update camera pos
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

		if (useFollowCamera)
		{
			glm::vec3 flatLookingDirection = cameraTargetPosition - followCameraAnchorPosition;
			if (glm::length2(glm::vec2(flatLookingDirection.x, flatLookingDirection.z)) > 0.01f)
				lookingInput.x = -glm::degrees(std::atan2f(flatLookingDirection.x, flatLookingDirection.z));
		}
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
	followCameraAnchorPosition = cameraPosition + glm::quat(glm::radians(glm::vec3(0, -lookingInput.x, 0))) * glm::vec3(0, 0, followCameraAnchorDistance);

	//
	// Update movement
	//
	targetCharacterLeanValue = 0.0f;
	isMoving = false;

	physx::PxVec3 velocity(0.0f);

	//
	// @SPECIAL_SKILL: Normal
	//
	if (playerState == PlayerState::NORMAL)
	{
		if (((PlayerPhysics*)getPhysicsComponent())->getIsGrounded() &&
			!((PlayerPhysics*)getPhysicsComponent())->getIsSliding())		// @TODO: for some reason sliding and trying to jump doesn't work... why???
		{
			velocity = processGroundedMovement(movementVector);
		}
		else
		{
			velocity = processAirMovement(movementVector);
		}

		// Poll input for the jump (include debounce input)
		jumpInputDebounceTimer -= MainLoop::getInstance().deltaTime;
		if (InputManager::getInstance().on_jumpPressed)
			jumpInputDebounceTimer = jumpInputDebounceTime;

		// Update number of jumps performed
		human_numJumpsCurrent = (((PlayerPhysics*)getPhysicsComponent())->getIsGrounded() || jumpCoyoteTimer > 0.0f) ? 0 : glm::max(1, human_numJumpsCurrent);

		// Jump (but not if sliding)
		const bool jumpInput =				(!lockJumping && jumpInputDebounceTimer > 0.0f);
		const bool canJumpGrounded =		(jumpInput && jumpCoyoteTimer > 0.0f);
		const bool human_canJumpAirbourne = (jumpInput && GameState::getInstance().currentTransformation == Transformation::HUMAN && human_numJumpsCurrent < GameState::getInstance().human_numJumps);
		if (canJumpGrounded || human_canJumpAirbourne)
		{
			jumpInputDebounceTimer = -1.0f;
			jumpCoyoteTimer = -1.0f;
			velocity.y = jumpSpeed[GameState::getInstance().playerIsHoldingWater];
			GameState::getInstance().inputStaminaEvent(StaminaEvent::JUMP);
			((PlayerPhysics*)getPhysicsComponent())->setIsGrounded(false);
			((PlayerPhysics*)getPhysicsComponent())->setIsSliding(false);
			triggerAnimationStateReset = true;

			//
			// @SPECIAL_SKILL: @HUMAN: Human midair Jump
			//
			human_numJumpsCurrent++;
			if (human_canJumpAirbourne && glm::length2(movementVector) > 0.1f)
			{
				glm::vec2 flatVelocity = glm::normalize(movementVector) * groundRunSpeed;
				velocity.x = flatVelocity.x;
				velocity.z = flatVelocity.y;

				facingDirection = movementVector;
			}
		}

		if (InputManager::getInstance().on_attackWindupPressed)
		{
			weaponDrawn = true;
			weaponDrawnPrevIsGrounded = false;		// Just resetting this flag so you can brandish midair
			weaponDrawnStyle = ((PlayerPhysics*)getPhysicsComponent())->getIsGrounded() ? (isMoving ? 2 : 1) : 3;	// @NOTE: @WEAPON_DRAWN_STYLE: 1: Horizontal; 2: Vertical; 3: Midair;
		}

		if (isMoving)
			GameState::getInstance().inputStaminaEvent(StaminaEvent::MOVE, MainLoop::getInstance().deltaTime);
	}

	//
	// @SPECIAL_SKILL: @HUMAN: Wall climb
	//
	else if (playerState == PlayerState::WALL_CLIMB_HUMAN)
	{
		if (ps_wallClimbHumanData.climbTimer > 0.0f)
			velocity = physx::PxVec3(0, ps_wallClimbHumanData.climbVelocityY, 0);
		else
			velocity = ((PlayerPhysics*)getPhysicsComponent())->velocity;

		//
		// See if should revert to normal playerstate
		// @Copypasta
		//
		const float radiusWithPadding = ((PlayerPhysics*)getPhysicsComponent())->controller->getRadius() + 0.1f;
		const float raycastDistance = glm::sqrt(2.0f * radiusWithPadding * radiusWithPadding) + ps_ledgeGrabHumanData.checkWallPadding;		// This is supposed to be 45 degrees... so the algorithm is pythagoren's theorem. sqrt(x^2 + x^2)
		physx::PxRaycastBuffer hitInfo;
		bool hit = PhysicsUtils::raycast(getPhysicsComponent()->getGlobalPose().p, physx::PxVec3(facingDirection.x, 0.0f, facingDirection.y), raycastDistance, hitInfo) && hitInfo.hasBlock;
		if (hit && velocity.y > 0.0f)
		{
			glm::vec3 flatNormal = PhysicsUtils::toGLMVec3(hitInfo.block.normal);
			flatNormal.y = 0.0f;
			flatNormal = glm::normalize(flatNormal);

			facingDirection.x = -flatNormal.x;
			facingDirection.y = -flatNormal.z;

			physx::PxVec3 targetPosition = hitInfo.block.position + PhysicsUtils::toPxVec3(flatNormal) * (((PlayerPhysics*)getPhysicsComponent())->controller->getRadius() + ps_wallClimbHumanData.climbDistancePadding);
			physx::PxVec3 deltaPosition = targetPosition - getPhysicsComponent()->getGlobalPose().p;
			velocity.x = deltaPosition.x;
			velocity.z = deltaPosition.z;

			//
			// Do another raycast to see if should ledge grab  @COPYPASTA
			//
			glm::vec3 ledgeGrabPoint = PhysicsUtils::toGLMVec3(hitInfo.block.position);		// @NOTE: this is the wall hit position
			physx::PxVec3 ledgeGrabRaycastOrigin = hitInfo.block.position;
			ledgeGrabRaycastOrigin.y = getPhysicsComponent()->getGlobalPose().p.y + ps_ledgeGrabHumanData.checkLedgeFromCenterY;	// @COPYPASTA
			ledgeGrabRaycastOrigin += PhysicsUtils::toPxVec3(-flatNormal * ps_ledgeGrabHumanData.checkLedgeTuckin);
			hit =
				PhysicsUtils::raycast(
					ledgeGrabRaycastOrigin,
					physx::PxVec3(0.0f, -1.0f, 0.0f),
					ps_ledgeGrabHumanData.checkLedgeRayDistance,
					hitInfo
				) &&
				hitInfo.hasBlock;

			if (hit)
			{
				ledgeGrabPoint.y = hitInfo.block.position.y;	// @NOTE: this is the top of the ledge Y position. This combined with the X,Z of the wall, you get the wall edge, so this should be exactly the wall edge.

				// Check to make sure the ledge grab crevice is large enough
				hit = PhysicsUtils::raycast(hitInfo.block.position, physx::PxVec3(0.0f, 1.0f, 0.0f), ps_ledgeGrabHumanData.checkLedgeCreviceHeightMin, hitInfo) && hitInfo.hasBlock;
				if (!hit)
				{
					// There's a hit for ledgegrab! (And the crevice is empty for at least the min required height!
					ps_ledgeGrabHumanData.holdLedgePosition = ledgeGrabPoint;
					playerState = PlayerState::LEDGE_GRAB_HUMAN;
				}
			}
		}
		else
		{
			playerState = PlayerState::NORMAL;		// Return back to normal when gravity takes its toll or if there's no more wall to climb up (raycast fails)
			velocity = physx::PxVec3(0, ps_wallClimbHumanData.climbVelocityY, 0);		// Bonus one final push! (especially useful if you met the end of the wall)
		}

		ps_wallClimbHumanData.climbTimer -= MainLoop::getInstance().deltaTime;
	}

	//
	// @SPECIAL_SKILL: @HUMAN: Ledge grab
	//
	else if (playerState == PlayerState::LEDGE_GRAB_HUMAN)
	{
		const physx::PxCapsuleController* controller = ((PlayerPhysics*)getPhysicsComponent())->controller;
		const glm::vec3 targetPosition =
			ps_ledgeGrabHumanData.holdLedgePosition +
			(controller->getRadius() + ps_ledgeGrabHumanData.holdLedgeOffset.x) *
				glm::vec3(-facingDirection.x, 0.0f, -facingDirection.y) +
			glm::vec3(0.0f, ps_ledgeGrabHumanData.holdLedgeOffset.y, 0.0f);
		velocity =
			PhysicsUtils::toPxVec3(targetPosition) -
			PhysicsUtils::toPxVec3(controller->getPosition());

		// Jump out of the ledge grab
		if (InputManager::getInstance().on_jumpPressed)
		{
			velocity.y = ps_ledgeGrabHumanData.jumpSpeed;
			GameState::getInstance().inputStaminaEvent(StaminaEvent::JUMP);
			playerState = PlayerState::NORMAL;
		}

		// @TODO: Hold the stick away from facingDirection to do a let go instead of a jump up.
		// @TODO: SCRATCH THAT, you should be able to jump when you press A, so pressing B should be let go. If you hold back (away from facingDirection) and press A, you should do a jump away from the ledge.)
	}

	//
	// @SPECIAL_SKILL: @CAT: Scale the wall
	//
	else if (playerState == PlayerState::WALL_SCALE_CAT)
	{
		std::cout << "Hello" << std::endl;
	}

	//characterLeanValue = targetCharacterLeanValue;		// @DEBUG: for testing the amount that the character leans
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
	const float runSpeed = weaponDrawn ? weaponDrawnRunSpeed : groundRunSpeed;

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
	// Setup the velocity
	//
	float movementVectorLength = glm::length2(movementVector);
	if (movementVectorLength > 0.001f)
		isMoving = true;
	else
		movementVectorLength = 0.0f;

	// Update Running Speed
	float targetRunningSpeed = movementVectorLength * runSpeed;
	const glm::vec2 accelerationDeceleration = weaponDrawn ? weaponDrawnAccelerationDeceleration : groundAccelerationDeceleration;

	currentRunSpeed = PhysicsUtils::moveTowards(
		currentRunSpeed,
		targetRunningSpeed,
		(currentRunSpeed >= 0.0f && currentRunSpeed < targetRunningSpeed ? accelerationDeceleration.x : accelerationDeceleration.y) *		// NOTE: when doing the turnaround and currentRunSpeed = -currentRunSpeed, the target running speed is technically > currentRunSpeed, so you get the acceleration coefficient, however, we want the deceleration one, bc the player is scooting backwards. It should just be that way.  -Timo
		MainLoop::getInstance().deltaTime
	);

	glm::vec3 velocity = glm::vec3(facingDirection.x, 0, facingDirection.y) * currentRunSpeed;

	// Add on a grounded rotation based on the ground you're standing on (if not isSliding)
	if (!((PlayerPhysics*)getPhysicsComponent())->getIsSliding())
	{
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

	//
	// Update the facing direction (if moving)
	//
	if (!lockFacingDirection)
	{
		const bool isSpinnySpinny = glm::abs(weaponDrawnSpinAccumulated) > weaponDrawnSpinAmountThreshold;
		if (!isSpinnySpinny && isMoving)
		{
			physx::PxVec3 velocityCopy = ((PlayerPhysics*)getPhysicsComponent())->velocity;
			velocityCopy.y = 0.0f;
			float flatVelocityMagnitude = velocityCopy.magnitude();

			// Skid stop behavior: essentially the character will quickly do
			// a 180 and then the velocity is retained when turning around.
			float mvtDotFacing = glm::dot(movementVector, facingDirection);
			if (!weaponDrawn && flatVelocityMagnitude > immediateTurningRequiredSpeed && mvtDotFacing < -0.707106781f)		// @RE:@NOTE: So after testing this, I decided that this shouldn't be allowed to work bc it makes starting up the spinny spinny much harder.  -Timo          ~~@NOTE: actually, after some asai thinking, I think the turnaround from the dot product should be allowed with the weapon drawn.  -Timo~~
			{
				facingDirection = movementVector;
				currentRunSpeed = -currentRunSpeed; // NOTE: this makes more sense especially when you land on the ground and inherit a butt ton of velocity
			}
			// Immediate facing direction when moving slowly:
			else if (!weaponDrawn && flatVelocityMagnitude <= immediateTurningRequiredSpeed)		// @NOTE: after some thinking, I really don't think that this should be allowed while the weapon is drawn, bc then turning would be very difficult.  -Timo
			{
				facingDirection = movementVector;
			}
			// Slowly turn towards your target
			else
			{
				//
				// Slowly face towards the targetFacingDirection
				//
				float facingDirectionAngle = glm::degrees(std::atan2f(facingDirection.x, facingDirection.y));
				float targetDirectionAngle = glm::degrees(std::atan2f(movementVector.x, movementVector.y));

				const float facingTurningAttenuation = glm::clamp(-(flatVelocityMagnitude - runSpeed) / (groundRunSpeedCantTurn - runSpeed) + 1.0f, 0.0f, 1.0f);		// https://www.desmos.com/calculator/fkrhuwn3l4
				const float turnSpeed = weaponDrawn ? weaponDrawnSpinSpeedMinMax.x : groundedFacingTurnSpeed;
				const float maxTurnSpeed = facingTurningAttenuation * turnSpeed * MainLoop::getInstance().deltaTime;
				float newFacingDirectionAngle = glm::radians(PhysicsUtils::moveTowardsAngle(facingDirectionAngle, targetDirectionAngle, maxTurnSpeed));
				facingDirection = glm::vec2(std::sinf(newFacingDirectionAngle), std::cosf(newFacingDirectionAngle));

				//
				// Calculate lean amount (use targetDirectionAngle bc of lack of deltaTime mess)
				//
				float deltaFacingDirectionAngle = targetDirectionAngle - facingDirectionAngle;

				if (deltaFacingDirectionAngle < -180.0f)			deltaFacingDirectionAngle += 360.0f;
				else if (deltaFacingDirectionAngle > 180.0f)		deltaFacingDirectionAngle -= 360.0f;

				// Lean amount
				const float deadZoneDeltaAngle = 0.1f;
				if (std::abs(deltaFacingDirectionAngle) > deadZoneDeltaAngle)
					targetCharacterLeanValue = std::clamp(-deltaFacingDirectionAngle * leanMultiplier / (60.0f * MainLoop::getInstance().deltaTime), -1.0f, 1.0f);
				else
					targetCharacterLeanValue = 0.0f;
			}
		}

		// Let's lean like Mikey Jackson when spinnyspinny
		if (isSpinnySpinny)
			targetCharacterLeanValue = -glm::sign(weaponDrawnSpinAccumulated);

		//
		// Accumulate Spinny Amount
		//
		if (weaponDrawn && isMoving)
		{
			const float facingDirectionAngle	= glm::degrees(std::atan2f(facingDirection.x, facingDirection.y));
			const float inputDirectionAngle		= glm::degrees(std::atan2f(movementVector.x, movementVector.y));
			float deltaDirectionAngle			= inputDirectionAngle - facingDirectionAngle;

			if (deltaDirectionAngle < -180.0f)			deltaDirectionAngle += 360.0f;
			else if (deltaDirectionAngle > 180.0f)		deltaDirectionAngle -= 360.0f;

			if (!isSpinnySpinny && glm::sign(weaponDrawnSpinAccumulated) != glm::sign(deltaDirectionAngle))
				weaponDrawnSpinAccumulated = 0.0f;		// Reset when starting to spin in the other direction (NOTE: only when not spinny spinny)

			// Accumulate the spinny!
			weaponDrawnSpinAccumulated += deltaDirectionAngle * MainLoop::getInstance().deltaTime;
			weaponDrawnSpinAccumulated = glm::clamp(weaponDrawnSpinAccumulated, -weaponDrawnSpinBuildupAmount, weaponDrawnSpinBuildupAmount);
		}

		//
		// Friction the Spinny Amount
		//
		if (!weaponDrawn || !isMoving)
		{
			if (!isSpinnySpinny)
			{
				weaponDrawnSpinAccumulated = 0.0f;
			}
			else
			{
				weaponDrawnSpinAccumulated =
					PhysicsUtils::moveTowards(
						weaponDrawnSpinAccumulated,
						0.0f,
						weaponDrawnSpinDeceleration * MainLoop::getInstance().deltaTime
					);
			}
		}

		//
		// Actually take over changing the facingDirection if Spinny spinny
		//
		if (isSpinnySpinny)
		{
			const float facingDirectionAngle = glm::degrees(std::atan2f(facingDirection.x, facingDirection.y));
			const float spinSpeedMinMaxLerpValue = (glm::abs(weaponDrawnSpinAccumulated) - weaponDrawnSpinAmountThreshold) / weaponDrawnSpinBuildupAmount;
			const float facingTurnSpeed = PhysicsUtils::lerp(weaponDrawnSpinSpeedMinMax.x, weaponDrawnSpinSpeedMinMax.y, spinSpeedMinMaxLerpValue);
			const float newFacingDirectionAngle = glm::radians(facingDirectionAngle + facingTurnSpeed * glm::sign(weaponDrawnSpinAccumulated) * MainLoop::getInstance().deltaTime);
			facingDirection = glm::vec2(std::sinf(newFacingDirectionAngle), std::cosf(newFacingDirectionAngle));
		}

		//std::cout << "SPINNYSPINNY: " << weaponDrawnSpinAccumulated << std::endl;
	}

	// Final package: Reset coyote time timer
	jumpCoyoteTimer = jumpCoyoteTime;
	ps_wallClimbHumanData.canEnterIntoState = true;		// Reset wallclimb since touched ground
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
	// Actually take over changing the facingDirection if Spinny spinny		@COPYPASTA with the grounded movement system.
	// @NOTE: with this, there is no resetting of the spinny spinny amount while midair. In fact, it doesn't get diminished while midair  -Timo
	//
	const bool isSpinnySpinny = glm::abs(weaponDrawnSpinAccumulated) > weaponDrawnSpinAmountThreshold;
	if (isSpinnySpinny)
	{
		const float facingDirectionAngle = glm::degrees(std::atan2f(facingDirection.x, facingDirection.y));
		const float spinSpeedMinMaxLerpValue = (glm::abs(weaponDrawnSpinAccumulated) - weaponDrawnSpinAmountThreshold) / weaponDrawnSpinBuildupAmount;
		const float facingTurnSpeed = PhysicsUtils::lerp(weaponDrawnSpinSpeedMinMax.x, weaponDrawnSpinSpeedMinMax.y, spinSpeedMinMaxLerpValue);
		const float newFacingDirectionAngle = glm::radians(facingDirectionAngle + facingTurnSpeed * glm::sign(weaponDrawnSpinAccumulated) * MainLoop::getInstance().deltaTime);
		facingDirection = glm::vec2(std::sinf(newFacingDirectionAngle), std::cosf(newFacingDirectionAngle));
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

	//
	// @SPECIAL_SKILL: @HUMAN: Wall climb & Ledge grab
	// @SPECIAL_SKILL: @CAT:   Wall scaling if rough walls! @TODO: figger
	//
	if ((GameState::getInstance().currentTransformation == Transformation::HUMAN ||
		GameState::getInstance().currentTransformation == Transformation::CAT) &&
		((PlayerPhysics*)getPhysicsComponent())->velocity.y < 0.0f)
	{
		// @copypasta
		const float radiusWithPadding = ((PlayerPhysics*)getPhysicsComponent())->controller->getRadius() + 0.1f;		// @COPYPASTA
		const float raycastDistance = glm::sqrt(2.0f * radiusWithPadding * radiusWithPadding) + ps_ledgeGrabHumanData.checkWallPadding;		// This is supposed to be 45 degrees... so the algorithm is pythagoren's theorem. sqrt(x^2 + x^2)
		physx::PxRaycastBuffer hitInfo;
		bool hit =
			PhysicsUtils::raycast(
				getPhysicsComponent()->getGlobalPose().p + physx::PxVec3(0, ps_ledgeGrabHumanData.checkWallFromCenterY, 0),
				physx::PxVec3(facingDirection.x, 0.0f, facingDirection.y),
				raycastDistance,
				hitInfo
			) &&
			hitInfo.hasBlock;

		if (hit)
		{
			glm::vec3 flatNormal = PhysicsUtils::toGLMVec3(hitInfo.block.normal);
			flatNormal.y = 0.0f;
			flatNormal = glm::normalize(flatNormal);

			facingDirection.x = -flatNormal.x;
			facingDirection.y = -flatNormal.z;

			if (GameState::getInstance().currentTransformation == Transformation::HUMAN)
			{
				glm::vec3 ledgeGrabPoint = PhysicsUtils::toGLMVec3(hitInfo.block.position);		// @NOTE: this is the wall hit position

				//
				// Do another raycast to see if should ledge grab  @COPYPASTA
				//
				physx::PxVec3 ledgeGrabRaycastOrigin = hitInfo.block.position;
				ledgeGrabRaycastOrigin.y = getPhysicsComponent()->getGlobalPose().p.y + ps_ledgeGrabHumanData.checkLedgeFromCenterY;	// @COPYPASTA
				ledgeGrabRaycastOrigin += PhysicsUtils::toPxVec3(-flatNormal * ps_ledgeGrabHumanData.checkLedgeTuckin);
				hit =
					PhysicsUtils::raycast(
						ledgeGrabRaycastOrigin,
						physx::PxVec3(0.0f, -1.0f, 0.0f),
						ps_ledgeGrabHumanData.checkLedgeRayDistance,
						hitInfo
					) &&
					hitInfo.hasBlock;

				if (hit)
				{
					ledgeGrabPoint.y = hitInfo.block.position.y;	// @NOTE: this is the top of the ledge Y position. This combined with the X,Z of the wall, you get the wall edge, so this should be exactly the wall edge.

					// Check to make sure the ledge grab crevice is large enough
					hit = PhysicsUtils::raycast(hitInfo.block.position, physx::PxVec3(0.0f, 1.0f, 0.0f), ps_ledgeGrabHumanData.checkLedgeCreviceHeightMin, hitInfo) && hitInfo.hasBlock;
					if (!hit)
					{
						// There's a hit for ledgegrab! (And the crevice is empty for at least the min required height!
						ps_ledgeGrabHumanData.holdLedgePosition = ledgeGrabPoint;
						playerState = PlayerState::LEDGE_GRAB_HUMAN;
					}
				}

				//
				// Enter into the wall climb state bc no ledge grab!
				//
				else if (ps_wallClimbHumanData.canEnterIntoState && glm::length2(currentFlatVelocity) > 0.001f)		// @NOTE: you need to be moving to do a wall climb, but not to do a ledge grab. Keep that in mind, yo.  -Timo
				{
					ps_wallClimbHumanData.canEnterIntoState = false;
					ps_wallClimbHumanData.climbTimer = ps_wallClimbHumanData.climbTime;
					playerState = PlayerState::WALL_CLIMB_HUMAN;
				}
			}
			else if (GameState::getInstance().currentTransformation == Transformation::CAT)
			{
				if (glm::length2(currentFlatVelocity) > 0.001f)
				{
					playerState = PlayerState::WALL_SCALE_CAT;
				}
			}
		}
	}

	// Deplete coyote time timer
	jumpCoyoteTimer -= MainLoop::getInstance().deltaTime;

	return currentVelocity;
}

void PlayerCharacter::processActions()
{
	if (GameState::getInstance().playerIsHoldingWater &&
		InputManager::getInstance().on_useItemPressed)
	{
		// Drink the water
		GameState::getInstance().playerIsHoldingWater = false;
		GameState::getInstance().currentPlayerStaminaAmount =
			(float)GameState::getInstance().maxPlayerStaminaAmount;

		triggerDrinkWaterAnimation = true;
	}
}

float hairWeightMult = 25.0f;				// @Debug
float speedAnimRunningMult = 1.3f;			// @Debug
float speedAnimRunningFloor = 0.525f;		// @Debug
void PlayerCharacter::processAnimation()
{
	constexpr int IDLE_ANIM					= 0;
	constexpr int WALKING_ANIM				= 1;
	constexpr int RUNNING_ANIM				= 2;
	constexpr int JUMP_ANIM					= 3;
	constexpr int LAND_ANIM					= 6;
	constexpr int DRAW_WATER_ANIM			= 10;
	constexpr int DRINK_WATER_ANIM			= 11;
	constexpr int SHEATH_BOTTLE_ANIM		= 12;
	constexpr int WRITE_IN_JOURNAL_ANIM		= 13;
	constexpr int HUMAN_SPEC_WALL_CLIMB		= 14;
	constexpr int HUMAN_SPEC_WALL_HANG		= 15;
	constexpr int IDLE_WEAPON_HORIZONTAL	= 16;
	constexpr int IDLE_WEAPON_VERTICAL		= 17;
	constexpr int ATTACK_LIGHT_HORIZONTAL	= 18;
	constexpr int ATTACK_LIGHT_VERTICAL		= 19;
	constexpr int ATTACK_MIDAIR				= 20;
	constexpr int SPINNY_SPINNY_ANIM		= 21;
	constexpr int IDLE_SPINNY_LEFT_ANIM		= 22;
	constexpr int IDLE_SPINNY_RIGHT_ANIM	= 23;

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
	else if (animationState == 7)
	{
		// Lock movement
		((PlayerPhysics*)getPhysicsComponent())->lockVelocity(false);
		lockFacingDirection = true;
		lockJumping = true;

		// Wait until write_in_journal animation is finished
		if (animator.isAnimationFinished(WRITE_IN_JOURNAL_ANIM, MainLoop::getInstance().deltaTime))
		{
			// Pick up bottle
			animationState = 6;
		}
	}
	else if (animationState == 8)
	{
		if (playerState != PlayerState::WALL_CLIMB_HUMAN && ((PlayerPhysics*)getPhysicsComponent())->getIsGrounded())
			animationState = 3;
	}
	else if (animationState == 9)
	{
		if (playerState != PlayerState::LEDGE_GRAB_HUMAN)
			animationState = 0;
	}
	else if (animationState == 10)
	{
		bool isGrounded = ((PlayerPhysics*)getPhysicsComponent())->getIsGrounded();

		if (isGrounded && weaponDrawnStyle == 3)
			weaponDrawnStyle = 2;		// Revert back to the Vertical swing (if didn't unleash the swing while midair)

		if (!InputManager::getInstance().attackWindupPressed)
		{
			// Stopped drawing the weapon
			weaponDrawn = false;
			animationState = 0;
			triggerAnimationStateReset = true;
		}
		else if (weaponDrawnPrevIsGrounded && !isGrounded)
			// Jump
			animationState = 2;
		else if (InputManager::getInstance().on_attackUnleashPressed)
			// Unleash attack
			animationState = 11;

		weaponDrawnPrevIsGrounded = isGrounded;
	}
	else if (animationState == 11)
	{
		bool isFinished = false;
		if (weaponDrawnStyle == 1)
			isFinished = animator.isAnimationFinished(ATTACK_LIGHT_HORIZONTAL, MainLoop::getInstance().deltaTime);
		else if (weaponDrawnStyle == 2)
			isFinished = animator.isAnimationFinished(ATTACK_LIGHT_VERTICAL, MainLoop::getInstance().deltaTime);
		else if (weaponDrawnStyle == 3)
			isFinished = animator.isAnimationFinished(ATTACK_MIDAIR, MainLoop::getInstance().deltaTime);

		if (isFinished)
			// Go back to having the weaponDrawn
			animationState = 10;
	}

	//
	// Overriding animation states
	//
	if (playerState == PlayerState::NORMAL && InputManager::getInstance().on_attackWindupPressed /*&& ((PlayerPhysics*)getPhysicsComponent())->getIsGrounded()*/)
	{
		animationState = 10;		// @NOTE: this simply *starts* the weapon holding animation
	}

	if (animationState != 10 && animationState != 11)
	{
		weaponDrawn = false;		// @NOTE: this forces weaponDrawn to be false when the animation state changes. You have to do RT/LClick again to get weaponDrawn=true
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

	if (InputManager::getInstance().pausePressed)		// @TODO: this should be on_ but where the actual "saving in the journal" happens is in physicsupdate. So refactor it outta there first of all!
		animationState = 7;

	if (playerState == PlayerState::WALL_CLIMB_HUMAN)
		animationState = 8;

	if (playerState == PlayerState::LEDGE_GRAB_HUMAN)
		animationState = 9;

	//
	// Update Animation State
	//
	const bool isSpinnySpinny = glm::abs(weaponDrawnSpinAccumulated) > weaponDrawnSpinAmountThreshold;
	if (prevIsSpinnySpinny != isSpinnySpinny)
		triggerAnimationStateReset = true;
	prevIsSpinnySpinny = isSpinnySpinny;

	if (triggerAnimationStateReset ||
		prevAnimState != animationState)
	{
		triggerAnimationStateReset = false;		// Reset flag

		switch (animationState)
		{
		case 0:
			// Idle
			if (isSpinnySpinny)
				animator.playAnimation(weaponDrawnSpinAccumulated > 0.0f ? IDLE_SPINNY_LEFT_ANIM : IDLE_SPINNY_RIGHT_ANIM, 6.0f, false, true);
			else
				animator.playAnimation(IDLE_ANIM + (unsigned int)isMoving, 6.0f, true, true);

			break;

		case 1:
			// Move
			if (isSpinnySpinny)
				animator.playAnimation(IDLE_SPINNY_LEFT_ANIM, 6.0f, false, true);
			else
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
			if (human_numJumpsCurrent > 1)
				animator.playAnimation(JUMP_ANIM + 2, 0.0f, false, true);		// Midair jump
			else
				animator.playAnimation(JUMP_ANIM + (int)isMoving, 0.0f, false, true);
			break;

		case 3:
			// Land
			animator.playAnimation(LAND_ANIM + (int)isMoving, 0.0f, false, true);
			break;

		case 4:
			// Draw water
			animator.playAnimation(DRAW_WATER_ANIM, 1.0f, false, true);
			break;

		case 5:
			// Drink water
			animator.playAnimation(DRINK_WATER_ANIM, 1.0f, false, true);
			break;

		case 6:
			// Pick up bottle
			animator.playAnimation(SHEATH_BOTTLE_ANIM, 7.5f, false, true);
			break;

		case 7:
			// Write in journal
			animator.playAnimation(WRITE_IN_JOURNAL_ANIM, 0.0f, false, true);
			break;

		case 8:
			// @SPECIAL_SKILL: @HUMAN: Wall Climb
			animator.playAnimation(HUMAN_SPEC_WALL_CLIMB, 0.0f, false, true);
			break;

		case 9:
			// @SPECIAL_SKILL: @HUMAN: Ledge grab
			animator.playAnimation(HUMAN_SPEC_WALL_HANG, 0.0f, false, true);
			break;

		case 10:
			if (isSpinnySpinny)
			{
				animator.playAnimation(SPINNY_SPINNY_ANIM, 6.0f, false, true);
				break;		// Break early if figured out that I'm a spinny spinny!
			}	

			// Idle with weapon drawn
			// @IDEA: @TODO: have different circumstances to start in different weapon stances
			// 				Like, with vertical overhead stance, you need to move, but what if you brandished your weapon
			//				right as you landed? That'd probably be a very low stance to take the shock of the land, and you're
			//				all ready to swing the weapon high and up.
			// @RESPONSE: So I like this idea. Perhaps after a little bit, if you don't use the stance, it reverts back
			//				to the IDLE_WEAPON_HORIZONTAL stance? That'd be interesting. Especially with the landing-low-stance
			//				one, bc you're literally using the force of landing to explode upwards in a swing. Also, this explosive
			//				swing should be much faster than the default horizontal one. It just seems to make sense to me.
			//				The vertical stance you get if you get into stance while moving makes sense bc you want the weapon
			//				away from your legs if you're moving. Idk if this vertical move should be faster or more powerful
			//				in any way, but we shall see eh!  -Timo
			if (weaponDrawnStyle == 1)
				animator.playAnimation(IDLE_WEAPON_HORIZONTAL, 6.0f, true, true);
			else if (weaponDrawnStyle == 2 || weaponDrawnStyle == 3)
				animator.playAnimation(IDLE_WEAPON_VERTICAL, 6.0f, true, true);
			break;

		case 11:
			// Unleash the weapon stance
			if (weaponDrawnStyle == 1)
				animator.playAnimation(ATTACK_LIGHT_HORIZONTAL, 6.0f, false, true);
			else if (weaponDrawnStyle == 2)
				animator.playAnimation(ATTACK_LIGHT_VERTICAL, 6.0f, false, true);
			else if (weaponDrawnStyle == 3)
				animator.playAnimation(ATTACK_MIDAIR, 3.0f, false, true);
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
	//std::cout << animator.animationSpeed << std::endl;

	//
	// @TODO: Do IK (Forward and Backward Reaching Inverse Kinematics for a heuristic approach)
	//
#ifdef _DEVELOP
	if (MainLoop::getInstance().playMode)
#endif
	{
		const glm::mat4 globalTransform = getTransform() * model->localTransform;

		if (rightSideburn.isFirstTime || leftSideburn.isFirstTime)
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

			rightSideburn.isFirstTime = false;
			leftSideburn.isFirstTime = false;
		}
		else
		{
			//
			// Just reset/lock the first bone
			//
			static const glm::vec4 neutralPosition(0, 0, 0, 1);
			leftSideburn.setPointPosition(0, 0.05f, globalTransform * animator.getBoneTransformation("Hair Sideburn1.L").globalTransformation * neutralPosition);
			rightSideburn.setPointPosition(0, 0.05f, globalTransform * animator.getBoneTransformation("Hair Sideburn1.R").globalTransformation * neutralPosition);

			leftSideburn.simulateRope(hairWeightMult);
			rightSideburn.simulateRope(hairWeightMult);

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
		}

		//
		// Calculate the bottle transformation matrix
		// @@@TODO: fix the baseObject->getTransform() areas, bc the transformation hierarchy isn't established yet.
		if (animationState == 4 || animationState == 5 || animationState == 6 || animationState == 7 || weaponDrawn)
			bottleModel->localTransform = model->localTransform * animator.getBoneTransformation("Hand Attachment").globalTransformation * bottleHandModelMatrix;
		else
			bottleModel->localTransform = model->localTransform * animator.getBoneTransformation("Back Attachment").globalTransformation * bottleModelMatrix;

		// Calculate the fit AABB for just the water mesh inside the bottle
		RenderAABB cookedBounds =
			PhysicsUtils::fitAABB(
				bottleModelWaterMesh->bounds,
				getTransform() * bottleModel->localTransform
			);
		BottledWaterBobbingMaterial::getInstance().setTopAndBottomYWorldSpace(
			cookedBounds.center.y + cookedBounds.extents.y,
			cookedBounds.center.y - cookedBounds.extents.y
		);
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
bool showPlayerLedgeGrabAndWallClimbVisuals = false;

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
	ImGui::DragFloat2("Running Acceleration/Deceleration", &groundAccelerationDeceleration.x, 0.1f);
	ImGui::DragFloat2("Weapon Drawn Acceleration/Deceleration", &weaponDrawnAccelerationDeceleration.x, 0.1f);
	ImGui::DragFloat("Running Acceleration (Air)", &airAcceleration, 0.1f);
	ImGui::DragFloat("Ground Running Speed", &groundRunSpeed, 0.1f);
	ImGui::DragFloat("Weapon Drawn Running Speed", &weaponDrawnRunSpeed, 0.1f);
	ImGui::DragFloat2("Jump Speed", &jumpSpeed[0], 0.1f);
	ImGui::Text(("Facing Direction: (" + std::to_string(facingDirection.x) + ", " + std::to_string(facingDirection.y) + ")").c_str());
	ImGui::DragFloat("Leaning Lerp Time", &leanLerpTime);
	ImGui::DragFloat("Lean Multiplier", &leanMultiplier, 0.05f);

	ImGui::Separator();
	ImGui::DragFloat("Facing Movement Speed", &groundedFacingTurnSpeed, 0.1f);
	ImGui::DragFloat("Facing Movement Speed (Air)", &airBourneFacingTurnSpeed, 0.1f);
	ImGui::Checkbox("Is Weapon Drawn", &weaponDrawn);
	ImGui::DragFloat2("Weapon Drawn Spin Speed Min Max", &weaponDrawnSpinSpeedMinMax.x);
	ImGui::DragFloat("Weapon Drawn Spin Accumulated", &weaponDrawnSpinAccumulated);
	ImGui::DragFloat("Weapon Drawn Spin Amount Threshold", &weaponDrawnSpinAmountThreshold);
	ImGui::DragFloat("Weapon Drawn Spin Deceleration", &weaponDrawnSpinDeceleration);
	ImGui::DragFloat("Weapon Drawn Spin Buildup Amount", &weaponDrawnSpinBuildupAmount);

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
	ImGui::ColorPicker3("Body Zelly Color2", &((ZellyMaterial*)materials["Body"])->getColor2().x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
	ImGui::ColorPicker3("Hair Zelly Color", &((ZellyMaterial*)materials["Hair"])->getColor().x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
	ImGui::ColorPicker3("Hair Zelly Color2", &((ZellyMaterial*)materials["Hair"])->getColor2().x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

	ImGui::Separator();
	ImGui::DragFloat("Hair Weight", &hairWeightMult);

	ImGui::Separator();
	ImGui::Checkbox("Indoor check show visual", &showPlayerIndoorDetectionCapsuleOverlap);
	ImGui::DragFloat("Indoor check radius", &indoorOverlapCheckRadius);
	ImGui::DragFloat("Indoor check height", &indoorOverlapCheckHeight);
	ImGui::DragFloat("Indoor check offset y", &indoorOverlapCheckOffY);

	ImGui::Separator();
	ImGui::Checkbox("Ledge grab and wall climb vis", &showPlayerLedgeGrabAndWallClimbVisuals);
	ImGui::DragFloat("WC Climb Time", &ps_wallClimbHumanData.climbTime);
	ImGui::DragFloat("WC Climb Velo", &ps_wallClimbHumanData.climbVelocityY);
	ImGui::DragFloat("WC Climb Distance", &ps_wallClimbHumanData.climbDistancePadding);
	ImGui::DragFloat("LG Check wall Y", &ps_ledgeGrabHumanData.checkWallFromCenterY);
	ImGui::DragFloat("LG Check wall padding", &ps_ledgeGrabHumanData.checkWallPadding);
	ImGui::DragFloat("LG Check height Y", &ps_ledgeGrabHumanData.checkLedgeFromCenterY);
	ImGui::DragFloat("LG check height Depth", &ps_ledgeGrabHumanData.checkLedgeRayDistance);
	ImGui::DragFloat("LG Tuckin", &ps_ledgeGrabHumanData.checkLedgeTuckin);
	ImGui::DragFloat("LG crevice Height min", &ps_ledgeGrabHumanData.checkLedgeCreviceHeightMin);
	ImGui::DragFloat2("LG holdLedge Offset", &ps_ledgeGrabHumanData.holdLedgeOffset.x);
	ImGui::DragFloat("LG jumpspeed", &ps_ledgeGrabHumanData.jumpSpeed);

	
	
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

	if (showPlayerLedgeGrabAndWallClimbVisuals)
	{
		// WALL_CLIMB_HUMAN
		const float radiusWithPadding = ((PlayerPhysics*)getPhysicsComponent())->controller->getRadius() + 0.1f;		// @COPYPASTA
		const float raycastDistance = glm::sqrt(2.0f * radiusWithPadding * radiusWithPadding) + ps_ledgeGrabHumanData.checkWallPadding;
		PhysicsUtils::imguiRenderRay(PhysicsUtils::toGLMVec3(getPhysicsComponent()->getGlobalPose().p + physx::PxVec3(0, ps_ledgeGrabHumanData.checkWallFromCenterY, 0)), glm::vec3(facingDirection.x, 0.0f, facingDirection.y) * raycastDistance, ImColor(0.3f, 1.0f, 0.4f));

		// LEDGE_GRAB_HUMAN
		glm::vec3 ledgeGrabOrigin = PhysicsUtils::toGLMVec3(getPhysicsComponent()->getGlobalPose().p) + glm::vec3(facingDirection.x, 0.0f, facingDirection.y) * raycastDistance;
		ledgeGrabOrigin.y = getPhysicsComponent()->getGlobalPose().p.y + ps_ledgeGrabHumanData.checkLedgeFromCenterY;		// @COPYPASTA
		PhysicsUtils::imguiRenderRay(ledgeGrabOrigin, glm::vec3(0, -ps_ledgeGrabHumanData.checkLedgeRayDistance, 0), ImColor(0.67f, 0.87f, 0.247f));
		PhysicsUtils::imguiRenderRay(ledgeGrabOrigin, glm::vec3(facingDirection.x, 0.0f, facingDirection.y) * ps_ledgeGrabHumanData.checkLedgeTuckin, ImColor(0.2f, 0.254f, 0.941f));
		PhysicsUtils::imguiRenderRay(ledgeGrabOrigin, glm::vec3(0, ps_ledgeGrabHumanData.checkLedgeCreviceHeightMin, 0), ImColor(0.87f, 0.247f, 0.819f));
	}
}
#endif
