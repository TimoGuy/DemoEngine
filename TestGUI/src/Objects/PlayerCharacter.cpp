#include "PlayerCharacter.h"

#include <glm/glm.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include "../MainLoop/MainLoop.h"
#include "../RenderEngine/RenderEngine.resources/ShaderResources.h"
#include "../RenderEngine/RenderEngine.resources/TextureResources.h"
#include "../RenderEngine/RenderEngine.camera/Camera.h"

#include "../ImGui/imgui.h"


PlayerCharacter::PlayerCharacter()
{
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


void PlayerCharacter::physicsUpdate(float deltaTime)
{
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

	glm::mat4 modelMatrix = glm::mat4(1.0f);
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
	ImGui::DragFloat3("Player Position", )				// TODO: implement position here by first making a physx object for this to go off!
}
