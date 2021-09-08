#include "PlayerCharacter.h"

#include <glm/glm.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include "../MainLoop/MainLoop.h"
#include "../RenderEngine/RenderEngine.resources/ShaderResources.h"
#include "../RenderEngine/RenderEngine.resources/TextureResources.h"

#include "../ImGui/imgui.h"

void imguiRenderBoxCollider(glm::mat4 modelMatrix, physx::PxBoxGeometry& boxGeometry);
void imguiRenderSphereCollider(glm::mat4 modelMatrix, physx::PxSphereGeometry& sphereGeometry);
void imguiRenderCapsuleCollider(glm::mat4 modelMatrix, physx::PxCapsuleGeometry& capsuleGeometry);
void imguiRenderCharacterController(glm::mat4 modelMatrix, physx::PxCapsuleController& controller);
void imguiRenderCircle(glm::mat4 modelMatrix, float radius, glm::vec3 eulerAngles, glm::vec3 offset, unsigned int numVertices);
void imguiRenderSausage(glm::mat4 modelMatrix, float radius, float halfHeight, glm::vec3 eulerAngles, unsigned int numVertices);

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

	physx::PxCapsuleControllerDesc desc;
	desc.upDirection = tempUp;
	desc.material = MainLoop::getInstance().defaultPhysicsMaterial;
	desc.position = physx::PxExtendedVec3(0.0f, 100.0f, 0.0f);
	desc.radius = 5.0f;
	desc.height = 5.0f;
	desc.slopeLimit = 0.70710678118f;												// cosine of 45 degrees
	desc.nonWalkableMode = physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING;
	desc.isValid();
	controller = (physx::PxCapsuleController*)MainLoop::getInstance().physicsControllerManager->createController(desc);


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
	physx::PxControllerCollisionFlags collisionFlags = controller->move(physx::PxVec3(0.0f, -98.0f, 0.0f), 0.01f, deltaTime, NULL, NULL);

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
	physx::PxTransform transformCopy = transform;
	ImGui::DragFloat3("Player Position", &transform.p[0]);
	if (transformCopy.p != transform.p || !(transformCopy.q == transform.q))
	{
		reapplyTransform = true;
	}

	ImGui::DragFloat3("Box Collider Half Extents", &boxCollider.halfExtents[0]);

	ImGui::DragFloat3("Controller up direction", &tempUp[0]);
	controller->setUpDirection(tempUp.getNormalized());
}

void PlayerCharacter::renderImGui()
{
	//imguiRenderBoxCollider(modelMatrix, boxCollider);
	//imguiRenderCapsuleCollider(modelMatrix, capsuleCollider);
	imguiRenderCharacterController(modelMatrix, *controller);
}

void imguiRenderBoxCollider(glm::mat4 modelMatrix, physx::PxBoxGeometry& boxGeometry)
{
	physx::PxVec3 halfExtents = boxGeometry.halfExtents;
	std::vector<glm::vec4> points = {
		modelMatrix * glm::vec4(halfExtents.x, halfExtents.y, halfExtents.z, 1.0f),
		modelMatrix * glm::vec4(-halfExtents.x, halfExtents.y, halfExtents.z, 1.0f),
		modelMatrix * glm::vec4(halfExtents.x, -halfExtents.y, halfExtents.z, 1.0f),
		modelMatrix * glm::vec4(-halfExtents.x, -halfExtents.y, halfExtents.z, 1.0f),
		modelMatrix * glm::vec4(halfExtents.x, halfExtents.y, -halfExtents.z, 1.0f),
		modelMatrix * glm::vec4(-halfExtents.x, halfExtents.y, -halfExtents.z, 1.0f),
		modelMatrix * glm::vec4(halfExtents.x, -halfExtents.y, -halfExtents.z, 1.0f),
		modelMatrix * glm::vec4(-halfExtents.x, -halfExtents.y, -halfExtents.z, 1.0f),
	};
	std::vector<glm::vec3> screenSpacePoints;
	unsigned int indices[4][4] = {
		{0, 1, 2, 3},
		{4, 5, 6, 7},
		{0, 4, 2, 6},
		{1, 5, 3, 7}
	};
	for (unsigned int i = 0; i < points.size(); i++)
	{
		//
		// Convert to screen space
		//
		glm::vec3 point = glm::vec3(points[i]);
		glm::vec3 pointOnScreen = MainLoop::getInstance().camera.PositionToClipSpace(point);
		float clipZ = pointOnScreen.z;
		pointOnScreen /= clipZ;
		pointOnScreen.x = pointOnScreen.x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
		pointOnScreen.y = -pointOnScreen.y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;
		pointOnScreen.z = clipZ;		// Reassign for test later
		screenSpacePoints.push_back(pointOnScreen);
	}

	for (unsigned int i = 0; i < 4; i++)
	{
		//
		// Draw quads if in the screen
		//
		glm::vec3 ssPoints[] = {
			screenSpacePoints[indices[i][0]],
			screenSpacePoints[indices[i][1]],
			screenSpacePoints[indices[i][2]],
			screenSpacePoints[indices[i][3]]
		};
		if (ssPoints[0].z < 0.0f || ssPoints[1].z < 0.0f || ssPoints[2].z < 0.0f || ssPoints[3].z < 0.0f) continue;

		ImGui::GetBackgroundDrawList()->AddQuad(
			ImVec2(ssPoints[0].x, ssPoints[0].y),
			ImVec2(ssPoints[1].x, ssPoints[1].y),
			ImVec2(ssPoints[3].x, ssPoints[3].y),
			ImVec2(ssPoints[2].x, ssPoints[2].y),
			ImColor::HSV(0.39f, 0.88f, 0.92f),
			3.0f
		);
	}
}

void imguiRenderSphereCollider(glm::mat4 modelMatrix, physx::PxSphereGeometry& sphereGeometry)
{
	imguiRenderCircle(modelMatrix, sphereGeometry.radius, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), 16);
	imguiRenderCircle(modelMatrix, sphereGeometry.radius, glm::vec3(glm::radians(90.0f), 0.0f, 0.0f), glm::vec3(0.0f), 16);
	imguiRenderCircle(modelMatrix, sphereGeometry.radius, glm::vec3(0.0f, glm::radians(90.0f), 0.0f), glm::vec3(0.0f), 16);
}

void imguiRenderCapsuleCollider(glm::mat4 modelMatrix, physx::PxCapsuleGeometry& capsuleGeometry)
{
	// Capsules by default extend along the x axis
	imguiRenderSausage(modelMatrix, capsuleGeometry.radius, capsuleGeometry.halfHeight, glm::vec3(0.0f, 0.0f, 0.0f), 16);
	imguiRenderSausage(modelMatrix, capsuleGeometry.radius, capsuleGeometry.halfHeight, glm::vec3(glm::radians(90.0f), 0.0f, 0.0f), 16);
	imguiRenderCircle(modelMatrix, capsuleGeometry.radius, glm::vec3(0.0f, glm::radians(90.0f), 0.0f), glm::vec3(-capsuleGeometry.halfHeight, 0.0f, 0.0f), 16);
	imguiRenderCircle(modelMatrix, capsuleGeometry.radius, glm::vec3(0.0f, glm::radians(90.0f), 0.0f), glm::vec3(capsuleGeometry.halfHeight, 0.0f, 0.0f), 16);
}

void imguiRenderCharacterController(glm::mat4 modelMatrix, physx::PxCapsuleController& controller)
{
	// Like a vertical capsule (default extends along the y axis... or whichever direction is up)
	float halfHeight = controller.getHeight() / 2.0f;
	physx::PxVec3 upDirection = controller.getUpDirection();
	modelMatrix *= glm::toMat4(glm::quat(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(upDirection.x, upDirection.y, upDirection.z)));
	imguiRenderSausage(modelMatrix, controller.getRadius(), halfHeight, glm::vec3(0.0f, 0.0f, glm::radians(90.0f)), 16);
	imguiRenderSausage(modelMatrix, controller.getRadius(), halfHeight, glm::vec3(glm::radians(90.0f), 0.0f, glm::radians(90.0f)), 16);
	imguiRenderCircle(modelMatrix, controller.getRadius(), glm::vec3(glm::radians(90.0f), 0.0f, 0.0f), glm::vec3(0.0f, halfHeight, 0.0f), 16);
	imguiRenderCircle(modelMatrix, controller.getRadius(), glm::vec3(glm::radians(90.0f), 0.0f, 0.0f), glm::vec3(0.0f, -halfHeight, 0.0f), 16);
}

void imguiRenderCircle(glm::mat4 modelMatrix, float radius, glm::vec3 eulerAngles, glm::vec3 offset, unsigned int numVertices)
{
	std::vector<glm::vec4> ringVertices;
	float angle = 0;
	for (unsigned int i = 0; i < numVertices; i++)
	{
		// Set point
		glm::vec4 point = glm::vec4(std::cosf(angle) * radius, std::sinf(angle) * radius, 0.0f, 1.0f);
		ringVertices.push_back(modelMatrix * (glm::toMat4(glm::quat(eulerAngles)) * point + glm::vec4(offset, 0.0f)));

		// Increment angle
		angle += physx::PxTwoPi / (float)numVertices;
	}

	//
	// Convert to screen space
	//
	std::vector<ImVec2> screenSpacePoints;
	for (unsigned int i = 0; i < ringVertices.size(); i++)
	{
		glm::vec3 point = glm::vec3(ringVertices[i]);
		glm::vec3 pointOnScreen = MainLoop::getInstance().camera.PositionToClipSpace(point);
		float clipZ = pointOnScreen.z;
		if (clipZ < 0.0f) return;							// NOTE: this is to break away from any rings that are intersecting with the near space

		pointOnScreen /= clipZ;
		pointOnScreen.x = pointOnScreen.x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
		pointOnScreen.y = -pointOnScreen.y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;
		screenSpacePoints.push_back(ImVec2(pointOnScreen.x, pointOnScreen.y));
	}

	ImGui::GetBackgroundDrawList()->AddPolyline(
		&screenSpacePoints[0],
		screenSpacePoints.size(),
		ImColor::HSV(0.39f, 0.88f, 0.92f),
		ImDrawFlags_Closed,
		3.0f
	);
}

void imguiRenderSausage(glm::mat4 modelMatrix, float radius, float halfHeight, glm::vec3 eulerAngles, unsigned int numVertices)					// NOTE: this is very similar to the function imguiRenderCircle
{
	assert(numVertices % 2 == 0);		// Needs to be even number of vertices to work

	std::vector<glm::vec4> ringVertices;
	float angle = glm::radians(90.0f);
	for (unsigned int i = 0; i <= numVertices / 2; i++)
	{
		// Set point
		glm::vec4 point = glm::vec4(std::cosf(angle) * radius - halfHeight, std::sinf(angle) * radius, 0.0f, 1.0f);
		ringVertices.push_back(modelMatrix * glm::toMat4(glm::quat(eulerAngles)) * point);

		// Increment angle
		if (i < numVertices / 2)
			angle += physx::PxTwoPi / (float)numVertices;
	}

	for (unsigned int i = 0; i <= numVertices / 2; i++)
	{
		// Set point
		glm::vec4 point = glm::vec4(std::cosf(angle) * radius + halfHeight, std::sinf(angle) * radius, 0.0f, 1.0f);
		ringVertices.push_back(modelMatrix * glm::toMat4(glm::quat(eulerAngles)) * point);

		// Increment angle
		if (i < numVertices / 2)
			angle += physx::PxTwoPi / (float)numVertices;
	}

	//
	// Convert to screen space
	//
	std::vector<ImVec2> screenSpacePoints;
	for (unsigned int i = 0; i < ringVertices.size(); i++)
	{
		glm::vec3 point = glm::vec3(ringVertices[i]);
		glm::vec3 pointOnScreen = MainLoop::getInstance().camera.PositionToClipSpace(point);
		float clipZ = pointOnScreen.z;
		if (clipZ < 0.0f) return;							// NOTE: this is to break away from any rings that are intersecting with the near space

		pointOnScreen /= clipZ;
		pointOnScreen.x = pointOnScreen.x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
		pointOnScreen.y = -pointOnScreen.y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;
		screenSpacePoints.push_back(ImVec2(pointOnScreen.x, pointOnScreen.y));
	}

	ImGui::GetBackgroundDrawList()->AddPolyline(
		&screenSpacePoints[0],
		screenSpacePoints.size(),
		ImColor::HSV(0.39f, 0.88f, 0.92f),
		ImDrawFlags_Closed,
		3.0f
	);
}
