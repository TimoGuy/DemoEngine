#include "PlayerCharacter.h"

#include <glm/glm.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include "../MainLoop/MainLoop.h"
#include "../RenderEngine/RenderEngine.resources/Resources.h"
#include "../Utils/PhysicsUtils.h"
#include "../RenderEngine/RenderEngine.light/Light.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"



#include "../RenderEngine/RenderEngine.light/DirectionalLight.h"			// temp


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
			2.0f);
}

PlayerRender::PlayerRender(BaseObject* bo, Bounds* bounds) : RenderComponent(bo, bounds)
{
	refreshResources();
}

void PlayerRender::refreshResources()
{
	pbrShaderProgramId = *(GLuint*)Resources::getResource("shader;pbr");
	shadowPassSkinnedProgramId = *(GLuint*)Resources::getResource("shader;shadowPassSkinned");

	//
	// Model and animation loading
	// (NOTE: it's highly recommended to only use the glTF2 format for 3d models,
	// since Assimp's model loader incorrectly includes bones and vertices with fbx)
	//
	/*std::vector<Animation> tryModelAnimations;
	model = Model("res/slime_glb.glb", tryModelAnimations, { 0, 1, 2, 3, 4, 5 });
	animator = Animator(&tryModelAnimations);*/
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

	physx::PxControllerCollisionFlags collisionFlags = controller->move(physx::PxVec3(0.0f, -9.8f, 0.0f), 0.01f, deltaTime, NULL, NULL);

	pos = controller->getPosition();
	baseObject->transform = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, pos.z));
}

void PlayerRender::preRenderUpdate()
{

}

void PlayerRender::render(unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture)
{
#ifdef _DEBUG
	refreshResources();
#endif

	glUseProgram(pbrShaderProgramId);
	glUniformMatrix4fv(glGetUniformLocation(pbrShaderProgramId, "cameraMatrix"), 1, GL_FALSE, glm::value_ptr(MainLoop::getInstance().camera.calculateProjectionMatrix() * MainLoop::getInstance().camera.calculateViewMatrix()));

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

	//
	// Try to find a shadow map
	//
	const size_t MAX_LIGHTS = 4;
	const size_t numLights = std::min(MAX_LIGHTS, MainLoop::getInstance().lightObjects.size());
	glUniform1i(glGetUniformLocation(pbrShaderProgramId, "numLights"), numLights);
	glUniform3fv(glGetUniformLocation(pbrShaderProgramId, "viewPosition"), 1, &MainLoop::getInstance().camera.position[0]);

	GLuint shadowMapTextureIndex = 0;
	bool setupCSM = false;
	for (size_t i = 0; i < numLights; i++)
	{
		//
		// Figures out casting shadows
		//
		if (MainLoop::getInstance().lightObjects[i]->castsShadows)
		{
			const GLuint baseOffset = 7;
			glActiveTexture(GL_TEXTURE0 + baseOffset + shadowMapTextureIndex);

			if (!setupCSM && MainLoop::getInstance().lightObjects[i]->getLight().lightType == LightType::DIRECTIONAL)
			{
				glBindTexture(GL_TEXTURE_2D_ARRAY, MainLoop::getInstance().lightObjects[i]->shadowMapTexture);
				glUniform1i(glGetUniformLocation(pbrShaderProgramId, "csmShadowMap"), baseOffset + shadowMapTextureIndex);

				// DirectionalLight: Setup for csm rendering
				glUniform1i(glGetUniformLocation(pbrShaderProgramId, "cascadeCount"), ((DirectionalLightLight*)MainLoop::getInstance().lightObjects[i])->shadowCascadeLevels.size());
				for (size_t j = 0; j < ((DirectionalLightLight*)MainLoop::getInstance().lightObjects[i])->shadowCascadeLevels.size(); ++j)
				{
					glUniform1f(glGetUniformLocation(pbrShaderProgramId, ("cascadePlaneDistances[" + std::to_string(j) + "]").c_str()), ((DirectionalLightLight*)MainLoop::getInstance().lightObjects[i])->shadowCascadeLevels[j]);
				}
				glUniformMatrix4fv(
					glGetUniformLocation(pbrShaderProgramId, "cameraView"),
					1,
					GL_FALSE,
					glm::value_ptr(MainLoop::getInstance().camera.calculateViewMatrix())
				);
				glUniform1f(glGetUniformLocation(pbrShaderProgramId, "nearPlane"), MainLoop::getInstance().camera.zNear);
				glUniform1f(glGetUniformLocation(pbrShaderProgramId, "farPlane"), MainLoop::getInstance().lightObjects[i]->shadowFarPlane);

				// Set flag
				setupCSM = true;
			}
			else if (MainLoop::getInstance().lightObjects[i]->getLight().lightType == LightType::SPOT)
			{
				glBindTexture(GL_TEXTURE_2D, MainLoop::getInstance().lightObjects[i]->shadowMapTexture);
				glUniform1i(glGetUniformLocation(pbrShaderProgramId, ("spotLightShadowMaps[" + std::to_string(i) + "]").c_str()), baseOffset + shadowMapTextureIndex);
			}
			else if (MainLoop::getInstance().lightObjects[i]->getLight().lightType == LightType::POINT)
			{
				glBindTexture(GL_TEXTURE_CUBE_MAP, MainLoop::getInstance().lightObjects[i]->shadowMapTexture);
				glUniform1i(glGetUniformLocation(pbrShaderProgramId, ("pointLightShadowMaps[" + std::to_string(i) + "]").c_str()), baseOffset + shadowMapTextureIndex);
			}

			shadowMapTextureIndex++;
		}

		//
		// Setting up lights
		//
		LightComponent* light = MainLoop::getInstance().lightObjects[i];
		glm::vec3 lightDirection = glm::vec3(0.0f);

		if (light->getLight().lightType != LightType::POINT)
			lightDirection = glm::normalize(light->getLight().facingDirection);																									// NOTE: If there is no direction (magnitude: 0), then that means it's a spot light ... Check this first in the shader
		
		glm::vec4 lightPosition = glm::vec4(glm::vec3(light->baseObject->transform[3]), light->getLight().lightType == LightType::DIRECTIONAL ? 0.0f : 1.0f);					// NOTE: when a directional light, position doesn't matter, so that's indicated with the w of the vec4 to be 0
		glm::vec3 lightColorWithIntensity = light->getLight().color * light->getLight().colorIntensity;
		
		glUniform3fv(glGetUniformLocation(pbrShaderProgramId, ("lightDirections[" + std::to_string(i) + "]").c_str()), 1, &lightDirection[0]);
		glUniform4fv(glGetUniformLocation(pbrShaderProgramId, ("lightPositions[" + std::to_string(i) + "]").c_str()), 1, &lightPosition[0]);
		glUniform3fv(glGetUniformLocation(pbrShaderProgramId, ("lightColors[" + std::to_string(i) + "]").c_str()), 1, &lightColorWithIntensity[0]);
	}

	glActiveTexture(GL_TEXTURE0);

	//
	// Setup the transformation matrices and lights
	//
	glUniformMatrix4fv(
		glGetUniformLocation(pbrShaderProgramId, "modelMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(baseObject->transform)
	);
	glUniformMatrix3fv(
		glGetUniformLocation(pbrShaderProgramId, "normalsModelMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(baseObject->transform))))
	);

	model.render(pbrShaderProgramId);
}

void PlayerRender::renderShadow(GLuint programId)
{
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
}

void PlayerImGui::renderImGui()
{
	//imguiRenderBoxCollider(transform, boxCollider);
	//imguiRenderCapsuleCollider(transform, capsuleCollider);
	PhysicsUtils::imguiRenderCharacterController(baseObject->transform, *((PlayerPhysics*)((PlayerCharacter*)baseObject)->physicsComponent)->controller);
	ImGuiComponent::renderImGui();
}

void PlayerImGui::cloneMe()
{
	// TODO: figure this out...
	new PlayerCharacter();
}
