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



#include "../RenderEngine/RenderEngine.light/DirectionalLight.h"			// temp
#include "../RenderEngine/RenderEngine.light/PointLight.h"					// temp


PlayerCharacter::PlayerCharacter()
{
	transform = glm::mat4(1.0f);

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

void PlayerPhysics::physicsUpdate(float deltaTime)
{
	//if (reapplyTransform)
	//{
	//	reapplyTransform = false;
	//	body->setGlobalPose(transform);
	//}

	//// Convert the physx object to model matrix
	//transform = body->getGlobalPose();
	//{
	//	physx::PxMat44 mat4 = physx::PxMat44(transform);
	//	glm::mat4 newMat;
	//	newMat[0][0] = mat4[0][0];
	//	newMat[0][1] = mat4[0][1];
	//	newMat[0][2] = mat4[0][2];
	//	newMat[0][3] = mat4[0][3];

	//	newMat[1][0] = mat4[1][0];
	//	newMat[1][1] = mat4[1][1];
	//	newMat[1][2] = mat4[1][2];
	//	newMat[1][3] = mat4[1][3];

	//	newMat[2][0] = mat4[2][0];
	//	newMat[2][1] = mat4[2][1];
	//	newMat[2][2] = mat4[2][2];
	//	newMat[2][3] = mat4[2][3];

	//	newMat[3][0] = mat4[3][0];
	//	newMat[3][1] = mat4[3][1];
	//	newMat[3][2] = mat4[3][2];
	//	newMat[3][3] = mat4[3][3];

	//	modelMatrix = newMat;
	//}
	physx::PxExtendedVec3 pos = controller->getPosition();
	glm::mat4 newTransform = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, pos.z));
	if (!(newTransform == baseObject->transform))
	{
		glm::vec3 newPosition = PhysicsUtils::getPosition(baseObject->transform);
		controller->setPosition(physx::PxExtendedVec3(newPosition.x, newPosition.y, newPosition.z));
	}

	velocity.y -= 9.8f * deltaTime;
	physx::PxControllerCollisionFlags collisionFlags = controller->move(velocity, 0.01f, deltaTime, NULL, NULL);

	pos = controller->getPosition();
	baseObject->transform[3] = glm::vec4(pos.x, pos.y, pos.z, 1.0f);
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

	// Short circuit if so
	if (!MainLoop::getInstance().playMode)
	{
		lookingInput = glm::vec2(0, 0);
		return;
	}

	//
	// Update looking input
	//
	lookingInput += glm::vec2(deltaX, deltaY) * lookingSensitivity;
	lookingInput.x = fmodf(lookingInput.x, 360.0f);
	if (lookingInput.x < 0.0f)
		lookingInput.x += 360.0f;
	lookingInput.y = std::clamp(lookingInput.y, -1.0f, 1.0f);

	//
	// Update playercam pos
	//
	glm::quat lookingRotation(glm::radians(glm::vec3(lookingInput.y * 85.0f, -lookingInput.x, 0.0f)));
	playerCamera.position = PhysicsUtils::getPosition(baseObject->transform) + lookingRotation * playerCamOffset;
	playerCamera.orientation = glm::normalize(lookingRotation * -playerCamOffset);

	//
	// Movement
	//
	float mvtSpeed = 0.5f;
	glm::vec2 movementVector(0.0f);
	if (glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_W) == GLFW_PRESS) movementVector.y += mvtSpeed;
	if (glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_A) == GLFW_PRESS) movementVector.x -= mvtSpeed;
	if (glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_S) == GLFW_PRESS) movementVector.y -= mvtSpeed;
	if (glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_D) == GLFW_PRESS) movementVector.x += mvtSpeed;

	bool isMoving = false;
	if (glm::length2(movementVector) > 0.001f)
	{
		isMoving = true;
		movementVector = glm::normalize(movementVector);
	}

	glm::vec3 velocity =
		glm::normalize(glm::vec3(playerCamera.orientation.x, 0.0f, playerCamera.orientation.z)) * movementVector.y +
		glm::cross(playerCamera.orientation, playerCamera.up) * movementVector.x;

	((PlayerPhysics*)((PlayerCharacter*)baseObject)->physicsComponent)->velocity = physx::PxVec3(velocity.x, velocity.y, velocity.z);

	if (isMoving)
	{
		// Start facing towards movement direction
		float targetFacingDirection = glm::degrees(std::atan2f(velocity.x, velocity.z));
		facingDirection = PhysicsUtils::lerpAngleDegrees(facingDirection, targetFacingDirection, facingSpeed);

		baseObject->transform =
			glm::translate(glm::mat4(1.0f), PhysicsUtils::getPosition(baseObject->transform)) *
			glm::eulerAngleXYZ(0.0f, glm::radians(facingDirection), 0.0f) *
			glm::scale(glm::mat4(1.0f), PhysicsUtils::getScale(baseObject->transform));
	}

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
	glUniformMatrix4fv(glGetUniformLocation(pbrShaderProgramId, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(baseObject->transform));
	glUniformMatrix3fv(glGetUniformLocation(pbrShaderProgramId, "normalsModelMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(baseObject->transform)))));
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
		glm::value_ptr(baseObject->transform)
	);
	// TODO: add skeletal stuff too eh!
	model.render(programId);
}

void PlayerImGui::propertyPanelImGui()
{
	ImGui::InputText("Name", &name);
	ImGui::Separator();
	PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(baseObject->transform));
	glm::vec3 newPos = PhysicsUtils::getPosition(baseObject->transform);
	((PlayerPhysics*)((PlayerCharacter*)baseObject)->physicsComponent)->controller->setPosition(physx::PxExtendedVec3(newPos.x, newPos.y, newPos.z));

	ImGui::DragFloat3("Controller up direction", &((PlayerPhysics*)((PlayerCharacter*)baseObject)->physicsComponent)->tempUp[0]);
	((PlayerPhysics*)((PlayerCharacter*)baseObject)->physicsComponent)->controller->setUpDirection(((PlayerPhysics*)((PlayerCharacter*)baseObject)->physicsComponent)->tempUp.getNormalized());

	ImGui::Separator();
	ImGui::Text("Virtual Camera");
	ImGui::DragFloat3("VirtualCamPosition", &((PlayerRender*)((PlayerCharacter*)baseObject)->renderComponent)->playerCamOffset[0]);
	ImGui::DragFloat2("Looking Input", &((PlayerRender*)((PlayerCharacter*)baseObject)->renderComponent)->lookingInput[0]);
	ImGui::DragFloat2("Looking Sensitivity", &((PlayerRender*)((PlayerCharacter*)baseObject)->renderComponent)->lookingSensitivity[0]);
}

void PlayerImGui::renderImGui()
{
	//imguiRenderBoxCollider(transform, boxCollider);
	//imguiRenderCapsuleCollider(transform, capsuleCollider);
	PhysicsUtils::imguiRenderCharacterController(baseObject->transform, *((PlayerPhysics*)((PlayerCharacter*)baseObject)->physicsComponent)->controller);
	ImGuiComponent::renderImGui();
}
