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
			4.5f);
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

PlayerCharacter::~PlayerCharacter()
{
	delete renderComponent;
	delete physicsComponent;
	delete imguiComponent;
}

void PlayerPhysics::physicsUpdate()
{
	velocity.y -= 9.8f * MainLoop::getInstance().physicsDeltaTime;
	physx::PxControllerCollisionFlags collisionFlags = controller->move(velocity, 0.01f, MainLoop::getInstance().physicsDeltaTime, NULL, NULL);

	isGrounded = false;
	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN)
	{
		std::cout << "\tDown Collision";

		//
		// Check if actually grounded
		//
		physx::PxVec3 origin = PhysicsUtils::toPxVec3(controller->getFootPosition());				// [in] Ray origin
		physx::PxVec3 unitDir = physx::PxVec3(0, -1, 0);											// [in] Ray direction
		physx::PxReal maxDistance = 0.5f;															// [in] Raycast max distance
		physx::PxRaycastBuffer hit;																	// [out] Raycast results

		// Raycast against all static & dynamic objects (no filtering)
		// The main result from this call is the closest hit, stored in the 'hit.block' structure
		if (MainLoop::getInstance().physicsScene->raycast(origin, unitDir, maxDistance, hit) &&
			hit.block.normal.dot(physx::PxVec3(0, 1, 0)) > 0.707106781f)		// NOTE: 0.7... is cos(45deg)
		{
			velocity.y = 0.0f;		// Remove gravity
			isGrounded = true;		// Can jump now
			physx::PxVec3 normal = hit.block.normal;		// For ground movement information	
			groundedNormal = glm::vec3(normal.x, normal.y, normal.z);
		}
			
	}
	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_SIDES)
	{
		std::cout << "\tSide Collision";
	}
	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_UP)
	{
		std::cout << "\tAbove Collision";
		//velocity.y = 0.0f;		// Hit your head on the ceiling
	}

	std::cout << std::endl;

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
	previousMouseX = mouseX;
	previousMouseY = mouseY;

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
	float mvtSpeed = 0.5f;
	glm::vec2 movementVector(0.0f);
	if (glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_W) == GLFW_PRESS) movementVector.y += mvtSpeed;
	if (glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_A) == GLFW_PRESS) movementVector.x -= mvtSpeed;
	if (glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_S) == GLFW_PRESS) movementVector.y -= mvtSpeed;
	if (glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_D) == GLFW_PRESS) movementVector.x += mvtSpeed;

	// Remove input if not playmode
	if (!MainLoop::getInstance().playMode)
	{
		lookingInput = glm::vec2(0, 0);
		movementVector = glm::vec2(0, 0);
		facingDirection = 0.0f;
	}

	//
	// Update playercam pos
	//
	glm::quat lookingRotation(glm::radians(glm::vec3(lookingInput.y * 85.0f, -lookingInput.x, 0.0f)));
	playerCamera.position = PhysicsUtils::getPosition(baseObject->getTransform()) + lookingRotation * playerCamOffset;
	playerCamera.orientation = glm::normalize(lookingRotation * -playerCamOffset);

	//
	// Update Movement Vector
	//
	bool isMoving = false;
	if (glm::length2(movementVector) > 0.001f)
	{
		isMoving = true;
		movementVector = glm::normalize(movementVector);
	}

	glm::vec3 velocity =
		movementVector.y * groundRunSpeed * glm::normalize(glm::vec3(playerCamera.orientation.x, 0.0f, playerCamera.orientation.z))  +
		movementVector.x * groundRunSpeed * glm::normalize(glm::cross(playerCamera.orientation, playerCamera.up));
	if (((PlayerPhysics*)baseObject->getPhysicsComponent())->getIsGrounded())
	{
		// Add on a grounded rotation based on the ground you're standing on
		glm::quat slopeRotation = glm::rotation(
			glm::vec3(0, 1, 0),
			((PlayerPhysics*)baseObject->getPhysicsComponent())->getGroundedNormal()
		);
		velocity = slopeRotation * velocity;
	}

	// Fetch in physics y (overwriting) (HOWEVER, not if going down a slope's -y is lower than the reported y (ignore when going up/jumping))
	if (velocity.y < 0.0f)
		velocity.y = std::min(((PlayerPhysics*)baseObject->getPhysicsComponent())->velocity.y, velocity.y);
	else
		velocity.y = ((PlayerPhysics*)baseObject->getPhysicsComponent())->velocity.y;

	// Jump
	if (((PlayerPhysics*)baseObject->getPhysicsComponent())->getIsGrounded() &&
		glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_SPACE) == GLFW_PRESS)
		velocity.y = jumpSpeed;

	((PlayerPhysics*)baseObject->getPhysicsComponent())->velocity = physx::PxVec3(velocity.x, velocity.y, velocity.z);

	if (isMoving)
	{
		// Start facing towards movement direction
		float targetFacingDirection = glm::degrees(std::atan2f(velocity.x, velocity.z));
		facingDirection = PhysicsUtils::lerpAngleDegrees(facingDirection, targetFacingDirection, facingSpeed);
	}

	renderTransform =
		glm::translate(glm::mat4(1.0f), PhysicsUtils::getPosition(baseObject->getTransform())) *
		glm::eulerAngleXYZ(0.0f, glm::radians(facingDirection), 0.0f) *
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
	ImGui::DragFloat("Running Speed", &((PlayerRender*)baseObject->getRenderComponent())->groundRunSpeed);
	ImGui::DragFloat("Jump Speed", &((PlayerRender*)baseObject->getRenderComponent())->jumpSpeed);
}

void PlayerImGui::renderImGui()
{
	//imguiRenderBoxCollider(transform, boxCollider);
	//imguiRenderCapsuleCollider(transform, capsuleCollider);
	PhysicsUtils::imguiRenderCharacterController(baseObject->getTransform(), *((PlayerPhysics*)baseObject->getPhysicsComponent())->controller);
	ImGuiComponent::renderImGui();
}
