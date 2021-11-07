#include "PlayerCharacter.h"

#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include "Components/PhysicsComponents.h"
#include "../MainLoop/MainLoop.h"
#include "../RenderEngine/RenderEngine.manager/RenderManager.h"
#include "../RenderEngine/RenderEngine.resources/Resources.h"
#include "../Utils/PhysicsUtils.h"
#include "../RenderEngine/RenderEngine.light/Light.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"

#include <cmath>


double previousMouseX, previousMouseY;

PlayerCharacter::PlayerCharacter()
{
	bounds = new Bounds();
	bounds->center = glm::vec3(0.0f);
	bounds->extents = glm::vec3(3.0f, 3.0f, 3.0f);		// NOTE: bc the renderTransform is different from the real transform, the bounds have to be thicker to account for when spinning

	imguiComponent = new PlayerImGui(this, bounds);
	physicsComponent = new PlayerPhysics(this);
	renderComponent = new PlayerRender(this, bounds);
}

void PlayerCharacter::loadPropertiesFromJson(nlohmann::json& object)		// @Override
{
	BaseObject::loadPropertiesFromJson(object["baseObject"]);
	imguiComponent->loadPropertiesFromJson(object["imguiComponent"]);

	//
	// I'll take the leftover tokens then
	//
}

nlohmann::json PlayerCharacter::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();
	j["imguiComponent"] = imguiComponent->savePropertiesToJson();

	return j;
}

PlayerRender::PlayerRender(BaseObject* bo, Bounds* bounds) : RenderComponent(bo, bounds)
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

	pbrShaderProgramId = *(GLuint*)Resources::getResource("shader;pbr");

	//
	// Model and animation loading
	// (NOTE: it's highly recommended to only use the glTF2 format for 3d models,
	// since Assimp's model loader incorrectly includes bones and vertices with fbx)
	//
	model = (Model*)Resources::getResource("model;slimeGirl");
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

				"Back Attachment"
			}
		);

	//
	// Load materials
	//
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
	//materials["Hair"] = (Material*)Resources::getResource("material;pbrSlimeHair");
	materials["Hair"] = (Material*)Resources::getResource("material;pbrSlimeBelt");
	materials["Shoes"] = (Material*)Resources::getResource("material;pbrSlimeShoeWhite");
	materials["ShoeWhite2"] = (Material*)Resources::getResource("material;pbrSlimeShoeWhite2");
	materials["ShoeBlack"] = (Material*)Resources::getResource("material;pbrSlimeShoeBlack");
	materials["ShoeAccent"] = (Material*)Resources::getResource("material;pbrSlimeShoeAccent");

	materials["PlasticCap"] = (Material*)Resources::getResource("material;pbrSlimeVest");
	materials["SeeThruRubber"] = (Material*)Resources::getResource("material;pbrSlimeEye");
	materials["MetalStand"] = (Material*)Resources::getResource("material;pbrRustyMetal");
	materials["Straw"] = (Material*)Resources::getResource("material;pbrSlimeTights");

	materials["Sweater"]->setTilingAndOffset(glm::vec4(0.4, 0.4, 0, 0));
	materials["Vest"]->setTilingAndOffset(glm::vec4(0.6, 0.6, 0, 0));
	materials["Shoes"]->setTilingAndOffset(glm::vec4(0.5, 0.5, 0, 0));

	model->setMaterials(materials);
}

void PlayerRender::processMovement()
{
	//
	// Update Camera position based off new mousex/y pos's
	//
	double mouseX, mouseY;
	glfwGetCursorPos(MainLoop::getInstance().window, &mouseX, &mouseY);					// TODO: make this a centralized input update that occurs
	double deltaX = mouseX - previousMouseX;
	double deltaY = mouseY - previousMouseY;

	if (MainLoop::getInstance().camera.getLockedCursor())
	{
		//
		// Lock the cursor
		// @Refactor: this code should not be here. It should probs be in mainloop ya think????
		//
		previousMouseX = (int)MainLoop::getInstance().camera.width / 2;		// NOTE: when setting cursor position as a double, the getCursorPos() function is slightly off, making the camera slowly move upwards
		previousMouseY = (int)MainLoop::getInstance().camera.height / 2;
		glfwSetCursorPos(MainLoop::getInstance().window, previousMouseX, previousMouseY);
	}
	else
	{
		// Regular update the previousMouse position
		previousMouseX = mouseX;
		previousMouseY = mouseY;
	}

	//
	// Get looking input
	//
	lookingInput += glm::vec2(deltaX, deltaY) * lookingSensitivity;
	lookingInput.x = fmodf(lookingInput.x, 360.0f);
	if (lookingInput.x < 0.0f)
		lookingInput.x += 360.0f;
	lookingInput.y = std::clamp(lookingInput.y, -1.0f, 1.0f);

	//
	// Movement
	//
	glm::vec2 movementVector(0.0f);
	if (glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_W) == GLFW_PRESS) movementVector.y += 1.0f;
	if (glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_A) == GLFW_PRESS) movementVector.x -= 1.0f;
	if (glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_S) == GLFW_PRESS) movementVector.y -= 1.0f;
	if (glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_D) == GLFW_PRESS) movementVector.x += 1.0f;

	// Change input vector to camera view
	glm::vec3 ThreeDMvtVector =
		movementVector.x * glm::normalize(glm::cross(playerCamera.orientation, playerCamera.up)) +
		movementVector.y * glm::normalize(glm::vec3(playerCamera.orientation.x, 0.0f, playerCamera.orientation.z));
	movementVector.x = ThreeDMvtVector.x;
	movementVector.y = ThreeDMvtVector.z;

	if (glm::length2(movementVector) > 0.001f)
		movementVector = PhysicsUtils::clampVector(movementVector, 0.0f, 1.0f);

	// Remove input if not playmode
	if (!MainLoop::getInstance().playMode)
	{
		lookingInput = glm::vec2(0, 0);
		currentRunSpeed = 0.0f;
		facingDirection = glm::vec2(0, 1);
		movementVector = glm::vec2(0, 0);
	}

	//
	// Update playercam pos
	//
	const glm::vec3 cameraPointingToPosition = PhysicsUtils::getPosition(baseObject->getTransform()) + glm::vec3(playerCamOffset.x, playerCamOffset.y, 0);
	float cameraDistance = playerCamOffset.z;

	glm::quat lookingRotation(glm::radians(glm::vec3(lookingInput.y * 85.0f, -lookingInput.x, 0.0f)));
	playerCamera.orientation = lookingRotation * glm::vec3(0, 0, 1);

	{
		// Do raycast to see what the camera distance should be
		physx::PxRaycastBuffer hitInfo;
		const float hitDistancePadding = 0.75f;
		if (PhysicsUtils::raycast(PhysicsUtils::toPxVec3(cameraPointingToPosition), PhysicsUtils::toPxVec3(-playerCamera.orientation), std::abs(cameraDistance) + hitDistancePadding, hitInfo))
		{
			cameraDistance = -hitInfo.block.distance + hitDistancePadding;		// NOTE: must be negative distance since behind model
		}
	}

	const glm::vec3 depthOffset(0, 0, cameraDistance);
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

	characterLeanValue = PhysicsUtils::lerp(
		characterLeanValue,
		targetCharacterLeanValue,
		leanLerpTime * MainLoop::getInstance().deltaTime
	);

	((PlayerPhysics*)baseObject->getPhysicsComponent())->velocity = velocity;

	glm::vec3 modelPosition = PhysicsUtils::getPosition(baseObject->getTransform());
	modelPosition.y += modelOffsetY;

	renderTransform =
		glm::translate(glm::mat4(1.0f), modelPosition) *
		glm::eulerAngleXYZ(0.0f, std::atan2f(facingDirection.x, facingDirection.y), glm::radians(characterLeanValue * 20.0f)) *
		glm::scale(glm::mat4(1.0f), PhysicsUtils::getScale(baseObject->getTransform()));
}

physx::PxVec3 PlayerRender::processGroundedMovement(const glm::vec2& movementVector)
{
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
		physx::PxVec3 velocityCopy = ((PlayerPhysics*)baseObject->getPhysicsComponent())->velocity;
		velocityCopy.y = 0.0f;
		float mvtDotFacing = glm::dot(movementVector, facingDirection);
		if (velocityCopy.magnitude() <= immediateTurningRequiredSpeed ||			// NOTE: not using magnitudeSquared() could defs be an inefficiency yo
			mvtDotFacing < -0.707106781f)
		{
			//
			// "Skid stop" state
			//
			facingDirection = movementVector;
			currentRunSpeed *= mvtDotFacing;		// If moving in the opposite direction than the facingDirection was, then the dot product will be negative, making the speed look like a skid
		}
		else
		{
			//
			// Slowly face towards the targetfacingdirection
			//
			float facingDirectionAngle = glm::degrees(std::atan2f(facingDirection.x, facingDirection.y));
			float targetDirectionAngle = glm::degrees(std::atan2f(movementVector.x, movementVector.y));

			float newFacingDirectionAngle = glm::radians(PhysicsUtils::moveTowardsAngle(facingDirectionAngle, targetDirectionAngle, facingTurnSpeed * MainLoop::getInstance().deltaTime));

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

	//
	// Update Running Speed
	//
	float targetRunningSpeed = movementVectorLength * groundRunSpeed;
	currentRunSpeed = PhysicsUtils::moveTowards(
		currentRunSpeed,
		targetRunningSpeed,
		(currentRunSpeed < targetRunningSpeed ? groundAcceleration : groundDecceleration) *
		MainLoop::getInstance().deltaTime
	);

	//
	// Apply the maths onto the actual velocity vector now!
	//
	glm::vec3 velocity = glm::vec3(facingDirection.x, 0, facingDirection.y) * currentRunSpeed;

	if (((PlayerPhysics*)baseObject->getPhysicsComponent())->getIsSliding())
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
	if (!((PlayerPhysics*)baseObject->getPhysicsComponent())->getIsSliding() &&
		glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_SPACE) == GLFW_PRESS)
		velocity.y = jumpSpeed;

	return physx::PxVec3(velocity.x, velocity.y, velocity.z);
}

physx::PxVec3 PlayerRender::processAirMovement(const glm::vec2& movementVector)
{
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
	// Setup velocity manipulation
	//
	physx::PxVec3 currentVelocity = ((PlayerPhysics*)baseObject->getPhysicsComponent())->velocity;

	//
	// Just add velocity in a flat way
	//
	float accelAmount = glm::length(movementVector);		// [0,1] range due to clamping before this function
	currentVelocity.x += movementVector.x * accelAmount * airAcceleration * MainLoop::getInstance().deltaTime;
	currentVelocity.z += movementVector.y * accelAmount * airAcceleration * MainLoop::getInstance().deltaTime;

	//
	// Clamp the velocity
	//
	glm::vec2 flatNewVelocity(currentVelocity.x, currentVelocity.z);
	if (glm::length2(flatNewVelocity) > 0.001f)
	{
		flatNewVelocity = PhysicsUtils::clampVector(flatNewVelocity, 0.0f, groundRunSpeed);		// NOTE: groundRunSpeed is simply a placeholder... I think.
		currentVelocity.x = flatNewVelocity.x;
		currentVelocity.z = flatNewVelocity.y;
	}

	return currentVelocity;
}

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
		if (((PlayerPhysics*)baseObject->getPhysicsComponent())->getIsGrounded())
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
			float time = animator.getCurrentTime() + animator.getCurrentAnimation()->getTicksPerSecond() * MainLoop::getInstance().deltaTime * animationSpeed;
			float duration = animator.getCurrentAnimation()->getDuration();
			if (time >= duration)
				// Standing
				animationState = 0;
		}
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
			animator.playAnimation((unsigned int)isMoving, 6.0f);
			break;

		case 1:
			// Jump
			animator.playAnimation(2 + isMoving, 3.0f, false, true);
			break;

		case 2:
			// Land
			animator.playAnimation(4 + isMoving, 0.0f, false);
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
	if (MainLoop::getInstance().playMode)
	{
		if (rightSideburn.isFirstTime || leftSideburn.isFirstTime || backAttachment.isFirstTime)
		{
			//
			// Setup Rope simulations
			//
			std::vector<glm::vec3> rightSideburnPoints;
			const glm::vec4 neutralPosition(0, 0, 0, 1);
			rightSideburnPoints.push_back(getRenderTransform() * animator.getBoneTransformation("Hair Sideburn1.R").globalTransformation * neutralPosition);
			rightSideburnPoints.push_back(getRenderTransform() * animator.getBoneTransformation("Hair Sideburn2.R").globalTransformation * neutralPosition);
			rightSideburnPoints.push_back(getRenderTransform() * animator.getBoneTransformation("Hair Sideburn3.R").globalTransformation * neutralPosition);
			rightSideburnPoints.push_back(getRenderTransform() * animator.getBoneTransformation("Hair Sideburn4.R").globalTransformation * neutralPosition);
			rightSideburn.initializePoints(rightSideburnPoints);

			std::vector<glm::vec3> leftSideburnPoints;
			leftSideburnPoints.push_back(getRenderTransform() * animator.getBoneTransformation("Hair Sideburn1.L").globalTransformation * neutralPosition);
			leftSideburnPoints.push_back(getRenderTransform() * animator.getBoneTransformation("Hair Sideburn2.L").globalTransformation * neutralPosition);
			leftSideburnPoints.push_back(getRenderTransform() * animator.getBoneTransformation("Hair Sideburn3.L").globalTransformation * neutralPosition);
			leftSideburnPoints.push_back(getRenderTransform() * animator.getBoneTransformation("Hair Sideburn4.L").globalTransformation * neutralPosition);
			leftSideburn.initializePoints(leftSideburnPoints);

			std::vector<glm::vec3> backAttachmentPoints;
			backAttachmentPoints.push_back(getRenderTransform() * animator.getBoneTransformation("Back Attachment").globalTransformation * neutralPosition);
			backAttachmentPoints.push_back(getRenderTransform() * animator.getBoneTransformation("Back Attachment").globalTransformation * (neutralPosition + glm::vec4(0, -50, 0, 0)));
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
			leftSideburn.setPointPosition(0, getRenderTransform() * animator.getBoneTransformation("Hair Sideburn1.L").globalTransformation * neutralPosition);
			rightSideburn.setPointPosition(0, getRenderTransform() * animator.getBoneTransformation("Hair Sideburn1.R").globalTransformation * neutralPosition);
			backAttachment.setPointPosition(0, getRenderTransform()* animator.getBoneTransformation("Back Attachment").globalTransformation * neutralPosition);
			
			leftSideburn.simulateRope(10.0f);
			rightSideburn.simulateRope(10.0f);
			backAttachment.simulateRope(850.0f);

			//
			// Do ik calculations for sideburns
			//
			const glm::mat4 inverseRenderTransform = glm::inverse(getRenderTransform());

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

	prevIsGrounded = ((PlayerPhysics*)baseObject->getPhysicsComponent())->getIsGrounded();
}

PlayerCharacter::~PlayerCharacter()
{
	delete renderComponent;
	delete physicsComponent;
	delete imguiComponent;
}

void PlayerRender::preRenderUpdate()
{
	processMovement();
	processAnimation();
}

void PlayerRender::render()
{
#ifdef _DEBUG
	//refreshResources();			// @Broken: animator = Animator(&model.getAnimations()); ::: This line will recreate the animator every frame, which resets the animator's timer. Zannnen
#endif

	//
	// Update the animation bones in the render manager
	//
	std::vector<glm::mat4>* boneTransforms = animator.getFinalBoneMatrices();
	MainLoop::getInstance().renderManager->updateSkeletalBonesUBO(*boneTransforms);
	model->render(renderTransform, true);
}

void PlayerRender::renderShadow(GLuint programId)
{
	std::vector<glm::mat4>* boneTransforms = animator.getFinalBoneMatrices();
	MainLoop::getInstance().renderManager->updateSkeletalBonesUBO(*boneTransforms);
	glUniformMatrix4fv(glGetUniformLocation(programId, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(renderTransform));
	model->render(renderTransform, false);
}

void PlayerImGui::propertyPanelImGui()
{
	ImGui::InputText("Name", &name);
	ImGui::Separator();

	// @Broken: The commented out lines below are apparently illegal operations towards the physics component
	//PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(baseObject->getTransform()));
	//glm::vec3 newPos = PhysicsUtils::getPosition(baseObject->getTransform());
	//((PlayerPhysics*)baseObject->getPhysicsComponent())->controller->setPosition(physx::PxExtendedVec3(newPos.x, newPos.y, newPos.z));

	//ImGui::DragFloat3("Controller up direction", &((PlayerPhysics*)baseObject->getPhysicsComponent())->tempUp[0]);
	//((PlayerPhysics*)baseObject->getPhysicsComponent())->controller->setUpDirection(((PlayerPhysics*)baseObject->getPhysicsComponent())->tempUp.getNormalized());

	ImGui::Separator();
	ImGui::Text("Virtual Camera");
	ImGui::DragFloat3("VirtualCamPosition", &((PlayerRender*)baseObject->getRenderComponent())->playerCamOffset[0]);
	ImGui::DragFloat2("Looking Input", &((PlayerRender*)baseObject->getRenderComponent())->lookingInput[0]);
	ImGui::DragFloat2("Looking Sensitivity", &((PlayerRender*)baseObject->getRenderComponent())->lookingSensitivity[0]);

	ImGui::Separator();
	ImGui::Text("Movement Properties");
	ImGui::DragFloat("Running Acceleration", &((PlayerRender*)baseObject->getRenderComponent())->groundAcceleration, 0.1f);
	ImGui::DragFloat("Running Decceleration", &((PlayerRender*)baseObject->getRenderComponent())->groundDecceleration, 0.1f);
	ImGui::DragFloat("Running Acceleration (Air)", &((PlayerRender*)baseObject->getRenderComponent())->airAcceleration, 0.1f);
	ImGui::DragFloat("Running Speed", &((PlayerRender*)baseObject->getRenderComponent())->groundRunSpeed, 0.1f);
	ImGui::DragFloat("Jump Speed", &((PlayerRender*)baseObject->getRenderComponent())->jumpSpeed, 0.1f);
	ImGui::Text(("Facing Direction: (" + std::to_string(((PlayerRender*)baseObject->getRenderComponent())->facingDirection.x) + ", " + std::to_string(((PlayerRender*)baseObject->getRenderComponent())->facingDirection.y) + ")").c_str());
	ImGui::DragFloat("Leaning Lerp Time", &((PlayerRender*)baseObject->getRenderComponent())->leanLerpTime);
	ImGui::DragFloat("Lean Multiplier", &((PlayerRender*)baseObject->getRenderComponent())->leanMultiplier, 0.05f);
	ImGui::DragFloat("Facing Movement Speed", &((PlayerRender*)baseObject->getRenderComponent())->facingTurnSpeed, 0.1f);
	ImGui::DragFloat("Facing Movement Speed (Air)", &((PlayerRender*)baseObject->getRenderComponent())->airBourneFacingTurnSpeed, 0.1f);

	ImGui::Separator();
	ImGui::DragFloat("Model Offset Y", &((PlayerRender*)baseObject->getRenderComponent())->modelOffsetY, 0.05f);
	ImGui::DragFloat("Model Animation Speed", &((PlayerRender*)baseObject->getRenderComponent())->animationSpeed);
}

void PlayerImGui::renderImGui()
{
	//
	// Draw the velocity line
	// TODO: add in a drawline function in physicsUtils.h
	//
	glm::vec3 pos1 = MainLoop::getInstance().camera.PositionToClipSpace(PhysicsUtils::getPosition(baseObject->getTransform()));
	physx::PxVec3 velocity = ((PlayerPhysics*)baseObject->getPhysicsComponent())->velocity;
	glm::vec3 pos2 = MainLoop::getInstance().camera.PositionToClipSpace(PhysicsUtils::getPosition(baseObject->getTransform()) + glm::vec3(velocity.x, velocity.y, velocity.z));

	if (pos1.z > 0.0f && pos2.z > 0.0f)
	{
		pos1 /= pos1.z;
		pos1.x = pos1.x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
		pos1.y = -pos1.y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;

		pos2 /= pos2.z;
		pos2.x = pos2.x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
		pos2.y = -pos2.y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;

		ImGui::GetBackgroundDrawList()->AddLine(ImVec2(pos1.x, pos1.y), ImVec2(pos2.x, pos2.y), ImColor::HSV(0.1083f, 0.66f, 0.91f), 3.0f);
	}

	ImGuiComponent::renderImGui();
}



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
		points[i] += (points[i] - prevPoints[i]) * 60.0f * deltaTime + glm::vec3(0, -9.8f * gravityMultiplier * deltaTime * deltaTime, 0);		// TODO: figure out why the gravity term requires deltaTime * deltaTime instead of regular stuff huh
		
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
			//if (glm::length(points[j] - points[j + 1]) < distances[j])
			//	continue;

			// NOTE: ignore dumb warnings
			glm::vec3 midpoint = (points[j] + points[j + 1]) / 2.0f;
			glm::vec3 direction = glm::normalize(points[j] - points[j + 1]);
			if (j > 0)
				points[j] = midpoint + direction * distances[j] / 2.0f;
			points[j + 1] = midpoint - direction * distances[j] / 2.0f;
		}
	}
}
