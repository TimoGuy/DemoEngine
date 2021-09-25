#include "YosemiteTerrain.h"

#include <glm/glm.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include "../MainLoop/MainLoop.h"
#include "../RenderEngine/RenderEngine.resources/Resources.h"
#include "../Utils/PhysicsUtils.h"
#include "../RenderEngine/RenderEngine.light/Light.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"




#include "../RenderEngine/RenderEngine.light/DirectionalLight.h"			// tEMP

YosemiteTerrain::YosemiteTerrain()
{
	transform = glm::mat4(1.0f);

	bounds = new Bounds();
	bounds->center = glm::vec3(0.0f);
	bounds->extents = glm::vec3(1.0f, 1.0f, 1.0f);

	imguiComponent = new YosemiteTerrainImGui(this, bounds);
	renderComponent = new YosemiteTerrainRender(this, bounds);
}

YosemiteTerrainRender::YosemiteTerrainRender(BaseObject* bo, Bounds* bounds) : RenderComponent(bo, bounds)
{
	refreshResources();
}

void YosemiteTerrainRender::refreshResources()
{
	pbrShaderProgramId = *(GLuint*)Resources::getResource("shader;pbr");
	shadowPassProgramId = *(GLuint*)Resources::getResource("shader;shadowPass");

	model = *(Model*)Resources::getResource("model;yosemiteTerrain");

	pbrAlbedoTexture = *(GLuint*)Resources::getResource("texture;pbrAlbedo");
	pbrNormalTexture = *(GLuint*)Resources::getResource("texture;pbrNormal");
	pbrMetalnessTexture = *(GLuint*)Resources::getResource("texture;pbrMetalness");
	pbrRoughnessTexture = *(GLuint*)Resources::getResource("texture;pbrRoughness");
}

YosemiteTerrain::~YosemiteTerrain()
{
	delete renderComponent;
	delete imguiComponent;
}

void YosemiteTerrain::streamTokensForLoading(nlohmann::json& object)		// @Override
{
	BaseObject::streamTokensForLoading(object);
	imguiComponent->streamTokensForLoading(object);
	/*physicsComponent->streamTokensForLoading(object);
	renderComponent->streamTokensForLoading(object);*/

	//
	// I'll take the leftover tokens then
	//
}

void YosemiteTerrainRender::preRenderUpdate()
{
	
}


void YosemiteTerrainRender::render(unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture)
{
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
	for (size_t i = 0; i < MainLoop::getInstance().lightObjects.size(); i++)
	{
		if (!MainLoop::getInstance().lightObjects[i]->castsShadows)
			continue;

		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D_ARRAY, MainLoop::getInstance().lightObjects[i]->shadowMapTexture);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "shadowMap"), 7);
		break;
	}

	glActiveTexture(GL_TEXTURE0);


	//glm::mat4 modelMatrix = glm::mat4(1.0f);
		/*glm::translate(pbrModelPosition)
		* glm::scale(glm::mat4(1.0f), pbrModelScale);*/
	glUniformMatrix4fv(
		glGetUniformLocation(pbrShaderProgramId, "modelMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(baseObject->transform)
	);
	glUniformMatrix4fv(
		glGetUniformLocation(pbrShaderProgramId, "view"),
		1,
		GL_FALSE,
		glm::value_ptr(MainLoop::getInstance().camera.calculateViewMatrix())
	);
	glUniformMatrix3fv(
		glGetUniformLocation(pbrShaderProgramId, "normalsModelMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(baseObject->transform))))
	);
	glUniform1f(glGetUniformLocation(pbrShaderProgramId, "nearPlane"), MainLoop::getInstance().camera.zNear);
	glUniform1f(glGetUniformLocation(pbrShaderProgramId, "farPlane"), MainLoop::getInstance().lightObjects[0]->shadowFarPlane);

	const size_t MAX_LIGHTS = 4;
	const size_t numLights = std::min(MAX_LIGHTS, MainLoop::getInstance().lightObjects.size());
	for (unsigned int i = 0; i < numLights; i++)
	{
		LightComponent* light = MainLoop::getInstance().lightObjects[i];
		glm::vec3 lightDirection = glm::vec3(0.0f);
		if (light->getLight().lightType != LightType::POINT)
			lightDirection = glm::normalize(light->getLight().facingDirection);																// NOTE: If there is no direction (magnitude: 0), then that means it's a spot light ... Check this first in the shader
		glm::vec4 lightPosition = glm::vec4(glm::vec3(light->baseObject->transform[3]), light->getLight().lightType == LightType::DIRECTIONAL ? 0.0f : 1.0f);					// NOTE: when a directional light, position doesn't matter, so that's indicated with the w of the vec4 to be 0
		glm::vec3 lightColorWithIntensity = light->getLight().color * light->getLight().colorIntensity;
		glUniform3fv(glGetUniformLocation(pbrShaderProgramId, ("lightDirections[" + std::to_string(i) + "]").c_str()), 1, &lightDirection[0]);
		glUniform4fv(glGetUniformLocation(pbrShaderProgramId, ("lightPositions[" + std::to_string(i) + "]").c_str()), 1, &lightPosition[0]);
		glUniform3fv(glGetUniformLocation(pbrShaderProgramId, ("lightColors[" + std::to_string(i) + "]").c_str()), 1, &lightColorWithIntensity[0]);
	}
	glUniform1i(glGetUniformLocation(pbrShaderProgramId, "numLights"), numLights);

	glUniform3fv(glGetUniformLocation(pbrShaderProgramId, "viewPosition"), 1, &MainLoop::getInstance().camera.position[0]);

	model.render(pbrShaderProgramId);
}

void YosemiteTerrainRender::renderShadow(GLuint programId)
{
	glUniformMatrix4fv(
		glGetUniformLocation(programId, "modelMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(baseObject->transform)
	);
	model.render(programId);
}

void YosemiteTerrainImGui::propertyPanelImGui()
{
	ImGui::InputText("Name", &name);
	ImGui::Separator();
	PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(baseObject->transform));
}

void YosemiteTerrainImGui::renderImGui()
{
	//imguiRenderBoxCollider(transform, boxCollider);
	//imguiRenderCapsuleCollider(transform, capsuleCollider);
	ImGuiComponent::renderImGui();
}

void YosemiteTerrainImGui::cloneMe()
{
	// TODO: figure this out...
	new YosemiteTerrain();
}
