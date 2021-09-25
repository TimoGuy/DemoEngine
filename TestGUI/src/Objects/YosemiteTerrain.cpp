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
#include "../RenderEngine/RenderEngine.light/PointLight.h"					// temp

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

void YosemiteTerrain::loadPropertiesFromJson(nlohmann::json& object)		// @Override
{
	BaseObject::loadPropertiesFromJson(object["baseObject"]);
	imguiComponent->loadPropertiesFromJson(object["imguiComponent"]);

	//
	// I'll take the leftover tokens then
	//
}

nlohmann::json YosemiteTerrain::savePropertiesToJson()
{
	nlohmann::json j;
	j["type"] = TYPE_NAME;
	j["baseObject"] = BaseObject::savePropertiesToJson();
	j["imguiComponent"] = imguiComponent->savePropertiesToJson();

	return j;
}

void YosemiteTerrainRender::preRenderUpdate()
{
	
}


void YosemiteTerrainRender::render(unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture)
{
#ifdef _DEBUG
	refreshResources();
#endif
	// @Copypasta

	//
	// Reset shadow maps
	//
	const size_t MAX_LIGHTS = 4;
	for (size_t i = 0; i < MAX_LIGHTS; i++)
	{
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, "csmShadowMap"), 100);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, ("spotLightShadowMaps[" + std::to_string(i) + "]").c_str()), 100);
		glUniform1i(glGetUniformLocation(pbrShaderProgramId, ("pointLightShadowMaps[" + std::to_string(i) + "]").c_str()), 100);
	}

	//
	// Load in textures
	//
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
				glUniform1f(glGetUniformLocation(pbrShaderProgramId, ("pointLightShadowFarPlanes[" + std::to_string(i) + "]").c_str()), ((PointLightLight*)MainLoop::getInstance().lightObjects[i])->farPlane);
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
