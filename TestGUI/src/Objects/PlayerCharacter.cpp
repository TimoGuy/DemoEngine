#include "PlayerCharacter.h"

#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include "../MainLoop/MainLoop.h"
#include "../RenderEngine/RenderEngine.resources/Resources.h"
#include "../Utils/PhysicsUtils.h"
#include "../RenderEngine/RenderEngine.light/Light.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"

#include <cmath>



#include "../Objects/DirectionalLight.h"			// temp
#include "../Objects/PointLight.h"					// temp


PlayerCharacter::PlayerCharacter()
{
	bounds = new Bounds();
	bounds->center = glm::vec3(0.0f);
	bounds->extents = glm::vec3(2.0f, 3.0f, 1.0f);

	imguiComponent = new PlayerImGui(this, bounds);
	physicsComponent = new PlayerPhysics(this, bounds);
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

PlayerPhysics::PlayerPhysics(BaseObject* bo, Bounds* bounds) : PhysicsComponent(bo, bounds)
{
	controller =
		PhysicsUtils::createCapsuleController(
			MainLoop::getInstance().physicsControllerManager,
			MainLoop::getInstance().defaultPhysicsMaterial,
			physx::PxExtendedVec3(0.0f, 100.0f, 0.0f),
			1.0f,
			4.5f,
			this		// PxUserControllerHitReport
		);
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
	pbrShaderProgramId = *(GLuint*)Resources::getResource("shader;pbr");

	//
	// Model and animation loading
	// (NOTE: it's highly recommended to only use the glTF2 format for 3d models,
	// since Assimp's model loader incorrectly includes bones and vertices with fbx)
	//
	model = *(Model*)Resources::getResource("model;slimeGirl");
	animator = Animator(&model.getAnimations());

	pbrAlbedoTexture = *(GLuint*)Resources::getResource("texture;pbrAlbedo");
	pbrNormalTexture = *(GLuint*)Resources::getResource("texture;pbrNormal");
	pbrMetalnessTexture = *(GLuint*)Resources::getResource("texture;pbrMetalness");
	pbrRoughnessTexture = *(GLuint*)Resources::getResource("texture;pbrRoughness");
}

physx::PxVec3 PlayerRender::processGroundedMovement(const glm::vec2& movementVector)
{
	//
	// Set Movement Vector
	//
	const float movementVectorLength = glm::length(movementVector);
	if (movementVectorLength > 0.001f)
	{
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

			facingDirectionAngle = glm::radians(PhysicsUtils::moveTowardsAngle(facingDirectionAngle, targetDirectionAngle, facingTurnSpeed * MainLoop::getInstance().deltaTime));

			facingDirection = glm::vec2(std::sinf(facingDirectionAngle), std::cosf(facingDirectionAngle));
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

PlayerCharacter::~PlayerCharacter()
{
	delete renderComponent;
	delete physicsComponent;
	delete imguiComponent;
}

void PlayerPhysics::physicsUpdate()
{
	//
	// Add gravity (or sliding gravity if sliding)
	//
	velocity.y -= 9.8f * MainLoop::getInstance().physicsDeltaTime;

	physx::PxVec3 cookedVelocity = velocity;

	// (Last minute) convert -y to y along the face you're sliding down
	if (isSliding)
	{
		const glm::vec3 upXnormal = glm::cross(glm::vec3(0, 1, 0), currentHitNormal);
		const glm::vec3 uxnXnormal = glm::normalize(glm::cross(upXnormal, currentHitNormal));
		const glm::vec3 slidingVector = uxnXnormal * -velocity.y;

		const float flatSlidingUmph = 0.9f;			// NOTE: this is so that it's guaranteed that the character will also hit the ground the next frame, thus keeping the sliding state
		cookedVelocity.y = 0.0f;
		cookedVelocity += physx::PxVec3(
			slidingVector.x * flatSlidingUmph,
			slidingVector.y,
			slidingVector.z * flatSlidingUmph
		);
	}
	
	//
	// Do the deed
	//
	physx::PxControllerCollisionFlags collisionFlags = controller->move(cookedVelocity, 0.01f, MainLoop::getInstance().physicsDeltaTime, NULL, NULL);
	isGrounded = false;
	isSliding = false;			// @Check: see if the player is sliding down the hill if they're still considered "grounded" with this being flipped off at every step

	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN)
	{
		//std::cout << "\tDown Collision";
	
		////
		//// Check if actually grounded
		//// @Example: of a raycast in the scene, so note this down!!!!!
		////
		//physx::PxVec3 origin = PhysicsUtils::toPxVec3(controller->getFootPosition());				// [in] Ray origin
		//physx::PxVec3 unitDir = physx::PxVec3(0, -1, 0);											// [in] Ray direction
		//physx::PxReal maxDistance = 5.5f;															// [in] Raycast max distance
		//physx::PxRaycastBuffer hit;																	// [out] Raycast results
		//
		//// Raycast against all static & dynamic objects (no filtering)
		//// The main result from this call is the closest hit, stored in the 'hit.block' structure
		//if (MainLoop::getInstance().physicsScene->raycast(origin, unitDir, maxDistance, hit))
		//{
		//	
		//}

		isGrounded = true;
		if (glm::dot(currentHitNormal, glm::vec3(0, 1, 0)) > 0.707106781f)		// NOTE: 0.7... is cos(45deg)
		{
			velocity.y = 0.0f;		// Remove gravity
		}
		else
		{
			// Slide down!
			isSliding = true;
		}
	}
	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_SIDES)
	{
		//std::cout << "\tSide Collision";
	}
	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_UP)
	{
		//std::cout << "\tAbove Collision";
		velocity.y = 0.0f;		// Hit your head on the ceiling
	}
	//std::cout << std::endl;

	//
	// Apply transform
	//
	physx::PxExtendedVec3 pos = controller->getPosition();
	glm::mat4 trans = baseObject->getTransform();
	trans[3] = glm::vec4(pos.x, pos.y, pos.z, 1.0f);
	baseObject->INTERNALsubmitPhysicsCalculation(trans);
}

void PlayerPhysics::propagateNewTransform(const glm::mat4& newTransform)
{
	glm::vec3 pos = PhysicsUtils::getPosition(newTransform);
	controller->setPosition(physx::PxExtendedVec3(pos.x, pos.y, pos.z));
}

physx::PxTransform PlayerPhysics::getGlobalPose()
{
	physx::PxExtendedVec3 pos = controller->getPosition();
	return PhysicsUtils::createTransform(glm::vec3(pos.x, pos.y, pos.z));
}

double previousMouseX, previousMouseY;
void PlayerRender::preRenderUpdate()
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
		previousMouseX = MainLoop::getInstance().camera.width / 2;
		previousMouseY = MainLoop::getInstance().camera.height / 2;
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
	glm::quat lookingRotation(glm::radians(glm::vec3(lookingInput.y * 85.0f, -lookingInput.x, 0.0f)));
	playerCamera.position = PhysicsUtils::getPosition(baseObject->getTransform()) + lookingRotation * playerCamOffset;
	playerCamera.orientation = glm::normalize(lookingRotation * -playerCamOffset);

	physx::PxVec3 velocity(0.0f);
	if (((PlayerPhysics*)baseObject->getPhysicsComponent())->getIsGrounded())
		velocity = processGroundedMovement(movementVector);
	else
		velocity = processAirMovement(movementVector);

	((PlayerPhysics*)baseObject->getPhysicsComponent())->velocity = velocity;

	renderTransform =
		glm::translate(glm::mat4(1.0f), PhysicsUtils::getPosition(baseObject->getTransform())) *
		glm::eulerAngleXYZ(0.0f, std::atan2f(facingDirection.x, facingDirection.y), 0.0f) *
		glm::scale(glm::mat4(1.0f), PhysicsUtils::getScale(baseObject->getTransform()));

	//
	// Mesh Skinning
	// @Optimize: This line (takes "less than 7ms"), if run multiple times, will bog down performance like crazy. Perhaps implement gpu-based animation???? Or maybe optimize this on the cpu side.
	//
	animator.updateAnimation(MainLoop::getInstance().deltaTime * 42.0f);		// Correction: this adds more than 10ms consistently
}

void PlayerRender::render(unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture)
{
#ifdef _DEBUG
	//refreshResources();			// @Broken: animator = Animator(&model.getAnimations()); ::: This line will recreate the animator every frame, which resets the animator's timer. Zannnen
#endif

	// @Copypasta
	//
	// Load in textures
	//
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pbrAlbedoTexture);
	glUniform1i(glGetUniformLocation(pbrShaderProgramId, "albedoMap"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, pbrNormalTexture);
	glUniform1i(glGetUniformLocation(pbrShaderProgramId, "normalMap"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, pbrMetalnessTexture);
	glUniform1i(glGetUniformLocation(pbrShaderProgramId, "metallicMap"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, pbrRoughnessTexture);
	glUniform1i(glGetUniformLocation(pbrShaderProgramId, "roughnessMap"), 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	glUniform1i(glGetUniformLocation(pbrShaderProgramId, "irradianceMap"), 4);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	glUniform1i(glGetUniformLocation(pbrShaderProgramId, "prefilterMap"), 5);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
	glUniform1i(glGetUniformLocation(pbrShaderProgramId, "brdfLUT"), 6);

	glActiveTexture(GL_TEXTURE0);

	if (MainLoop::getInstance().playMode)
	{
		auto transforms = animator.getFinalBoneMatrices();			// @Copypasta
		for (size_t i = 0; i < transforms.size(); i++)
			glUniformMatrix4fv(
				glGetUniformLocation(pbrShaderProgramId, ("finalBoneMatrices[" + std::to_string(i) + "]").c_str()),
				1,
				GL_FALSE,
				glm::value_ptr(transforms[i])
			);
	}
	else
	{
		for (size_t i = 0; i < 100; i++)
			glUniformMatrix4fv(
				glGetUniformLocation(pbrShaderProgramId, ("finalBoneMatrices[" + std::to_string(i) + "]").c_str()),
				1,
				GL_FALSE,
				glm::value_ptr(glm::mat4(1.0f))
			);
	}

	//
	// Setup the transformation matrices and lights
	//
	glUniformMatrix4fv(glGetUniformLocation(pbrShaderProgramId, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(renderTransform));
	glUniformMatrix3fv(glGetUniformLocation(pbrShaderProgramId, "normalsModelMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(renderTransform)))));
	model.render(pbrShaderProgramId);
}

void PlayerRender::renderShadow(GLuint programId)
{
	auto transforms = animator.getFinalBoneMatrices();			// @Copypasta
	for (size_t i = 0; i < transforms.size(); i++)
		glUniformMatrix4fv(
			glGetUniformLocation(programId, ("finalBoneMatrices[" + std::to_string(i) + "]").c_str()),
			1,
			GL_FALSE,
			glm::value_ptr(transforms[i])
		);

	glUniformMatrix4fv(
		glGetUniformLocation(programId, "modelMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(renderTransform)
	);
	// TODO: add skeletal stuff too eh!
	model.render(programId);
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
	ImGui::DragFloat("Facing Movement Speed", &((PlayerRender*)baseObject->getRenderComponent())->facingTurnSpeed, 0.1f);
	ImGui::DragFloat("Facing Movement Speed (Air)", &((PlayerRender*)baseObject->getRenderComponent())->airBourneFacingTurnSpeed, 0.1f);
}

void PlayerImGui::renderImGui()
{
	//imguiRenderBoxCollider(transform, boxCollider);
	//imguiRenderCapsuleCollider(transform, capsuleCollider);
	
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

	PhysicsUtils::imguiRenderCharacterController(baseObject->getTransform(), *((PlayerPhysics*)baseObject->getPhysicsComponent())->controller);
	ImGuiComponent::renderImGui();
}

void PlayerPhysics::onShapeHit(const physx::PxControllerShapeHit& hit)
{
	currentHitNormal = glm::vec3(hit.worldNormal.x, hit.worldNormal.y, hit.worldNormal.z);

	//// @Checkin
	if (hit.worldNormal.dot(physx::PxVec3(0, 1, 0)) <= 0.707106781f)		// NOTE: 0.7... is cos(45deg)
	{
		physx::PxVec3 dtiith = hit.dir;
		float jjjjj = hit.length;
		//std::cout << dtiith << jjjjj << std::endl;
	}
}

void PlayerPhysics::onControllerHit(const physx::PxControllersHit& hit)
{
	//std::cout << "\t\t\t Contrller HIT" << std::endl;
}

void PlayerPhysics::onObstacleHit(const physx::PxControllerObstacleHit& hit)
{
	//std::cout << "\t\t\t SHAPE HIT" << std::endl;
}
