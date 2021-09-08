#include "PlayerCharacter.h"

#include <glm/glm.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include "../MainLoop/MainLoop.h"
#include "../RenderEngine/RenderEngine.resources/ShaderResources.h"
#include "../RenderEngine/RenderEngine.resources/TextureResources.h"
#include "../Physics/PhysicsUtils.h"


PlayerCharacter::PlayerCharacter()
{
	//transform = physx::PxTransform(physx::PxVec3(physx::PxReal(0), physx::PxReal(100), 0));
	//body = MainLoop::getInstance().physicsPhysics->createRigidDynamic(transform);
	//boxCollider = physx::PxBoxGeometry(3.0f, 3.0f, 3.0f);
	////physx::PxShape* shape = MainLoop::getInstance().physicsPhysics->createShape(boxCollider, *MainLoop::getInstance().defaultPhysicsMaterial);
	//capsuleCollider = physx::PxCapsuleGeometry(5.0f, 5.0f);
	//physx::PxShape* shape = MainLoop::getInstance().physicsPhysics->createShape(capsuleCollider, *MainLoop::getInstance().defaultPhysicsMaterial);
	//body->attachShape(*shape);
	//physx::PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
	//MainLoop::getInstance().physicsScene->addActor(*body);
	//shape->release();

	controller =
		PhysicsUtils::createCapsuleController(
			MainLoop::getInstance().physicsControllerManager,
			MainLoop::getInstance().defaultPhysicsMaterial,
			physx::PxExtendedVec3(0.0f, 100.0f, 0.0f),
			5.0f,
			5.0f);


	pbrShaderProgramId = ShaderResources::getInstance().setupShaderProgramVF("pbr", "pbr.vert", "pbr.frag");
	shadowPassSkinnedProgramId = ShaderResources::getInstance().setupShaderProgramVF("shadowPassSkinned", "shadow_skinned.vert", "do_nothing.frag");

	//
	// Model and animation loading
	// (NOTE: it's highly recommended to only use the glTF2 format for 3d models,
	// since Assimp's model loader incorrectly includes bones and vertices with fbx)
	//
	std::vector<Animation> tryModelAnimations;
	model = Model("res/slime_glb.glb", tryModelAnimations, { 0, 1, 2, 3, 4, 5 });
	animator = Animator(&tryModelAnimations);

	pbrAlbedoTexture = TextureResources::getInstance().loadTexture2D("pbrAlbedo", "res/rusted_iron/rustediron2_basecolor.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	pbrNormalTexture = TextureResources::getInstance().loadTexture2D("pbrNormal", "res/rusted_iron/rustediron2_normal.png", GL_RGB, GL_RGB, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	pbrMetalnessTexture = TextureResources::getInstance().loadTexture2D("pbrMetalness", "res/rusted_iron/rustediron2_metallic.png", GL_RED, GL_RED, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	pbrRoughnessTexture = TextureResources::getInstance().loadTexture2D("pbrAlbedo", "res/rusted_iron/rustediron2_roughness.png", GL_RED, GL_RED, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
}

PlayerCharacter::~PlayerCharacter()
{
}


glm::mat4 modelMatrix;
void PlayerCharacter::physicsUpdate(float deltaTime)
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
	physx::PxControllerCollisionFlags collisionFlags = controller->move(physx::PxVec3(0.0f, -9.8f, 0.0f), 0.01f, deltaTime, NULL, NULL);

	physx::PxExtendedVec3 pos = controller->getPosition();
	modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, pos.z));
}

void PlayerCharacter::render(bool shadowPass, unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture, unsigned int shadowMapTexture)
{
	if (shadowPass) return;		// FOR NOW
	unsigned int programId = shadowPass ? shadowPassSkinnedProgramId : pbrShaderProgramId;
	glUseProgram(programId);
	//glUniformMatrix4fv(glGetUniformLocation(programId, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightProjection * lightView));
	glUniformMatrix4fv(glGetUniformLocation(programId, "cameraMatrix"), 1, GL_FALSE, glm::value_ptr(MainLoop::getInstance().camera.calculateProjectionMatrix() * MainLoop::getInstance().camera.calculateViewMatrix()));


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pbrAlbedoTexture);
	glUniform1i(glGetUniformLocation(programId, "albedoMap"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, pbrNormalTexture);
	glUniform1i(glGetUniformLocation(programId, "normalMap"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, pbrMetalnessTexture);
	glUniform1i(glGetUniformLocation(programId, "metallicMap"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, pbrRoughnessTexture);
	glUniform1i(glGetUniformLocation(programId, "roughnessMap"), 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	glUniform1i(glGetUniformLocation(programId, "irradianceMap"), 4);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	glUniform1i(glGetUniformLocation(programId, "prefilterMap"), 5);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
	glUniform1i(glGetUniformLocation(programId, "brdfLUT"), 6);

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
	glUniform1i(glGetUniformLocation(programId, "shadowMap"), 7);

	glActiveTexture(GL_TEXTURE0);

	
	//glm::mat4 modelMatrix = glm::mat4(1.0f);
		/*glm::translate(pbrModelPosition)
		* glm::scale(glm::mat4(1.0f), pbrModelScale);*/
	glUniformMatrix4fv(
		glGetUniformLocation(programId, "modelMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(modelMatrix)
	);
	glUniformMatrix3fv(
		glGetUniformLocation(programId, "normalsModelMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(modelMatrix))))
	);

	glm::vec3 lightPosition = glm::vec3(5.0f, 5.0f, 5.0f);
	glUniform3fv(glGetUniformLocation(programId, "lightPositions[0]"), 1, &lightPosition[0]);
	glUniform3f(glGetUniformLocation(programId, "lightColors[0]"), 150.0f, 150.0f, 150.0f);
	glUniform3fv(glGetUniformLocation(programId, "viewPosition"), 1, &MainLoop::getInstance().camera.position[0]);

	model.render(programId);
}

void PlayerCharacter::propertyPanelImGui()
{
	physx::PxExtendedVec3 controllerPosition = controller->getPosition();
	float pos[3] = { controllerPosition.x, controllerPosition.y, controllerPosition.z };
	ImGui::DragFloat3("Player Position", pos);
	if (pos[0] != controllerPosition.x ||
		pos[1] != controllerPosition.y || 
		pos[2] != controllerPosition.z)
	{
		controller->setPosition(physx::PxExtendedVec3(pos[0], pos[1], pos[2]));
	}

	ImGui::DragFloat3("Controller up direction", &tempUp[0]);
	controller->setUpDirection(tempUp.getNormalized());
}

void PlayerCharacter::renderImGui()
{
	//imguiRenderBoxCollider(modelMatrix, boxCollider);
	//imguiRenderCapsuleCollider(modelMatrix, capsuleCollider);
	PhysicsUtils::imguiRenderCharacterController(modelMatrix, *controller);
}
