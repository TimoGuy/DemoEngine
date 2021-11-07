#include "Material.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include "../RenderEngine.resources/Resources.h"
#include "../RenderEngine.manager/RenderManager.h"
#include "../../MainLoop/MainLoop.h"

#include <iostream>

bool Material::resetFlag = false;

//
// Helper funks
//
GLuint currentShaderId = 0;
const glm::mat4* currentModelMatrix = nullptr;
void setupShader(GLuint shaderId, const glm::mat4* modelMatrix)
{
	//
	// Only update whatever is necessary (when shader changes, the uniforms need to be rewritten fyi)
	//
	bool changeAll = Material::resetFlag;
	if (changeAll || currentShaderId != shaderId)
	{
		changeAll = true;

		glUseProgram(shaderId);
		glUniformMatrix4fv(glGetUniformLocation(shaderId, "cameraMatrix"), 1, GL_FALSE, glm::value_ptr(MainLoop::getInstance().camera.calculateProjectionMatrix() * MainLoop::getInstance().camera.calculateViewMatrix()));
		MainLoop::getInstance().renderManager->setupSceneLights(shaderId);			// tODO: I'd really like to move to UBO's so we don't have to do this anymore
		
		currentShaderId = shaderId;
	}

	if (changeAll || currentModelMatrix != modelMatrix)
	{
		glUniformMatrix4fv(glGetUniformLocation(shaderId, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(*modelMatrix));
		glUniformMatrix3fv(glGetUniformLocation(shaderId, "normalsModelMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(*modelMatrix)))));

		currentModelMatrix = modelMatrix;
	}

	Material::resetFlag = false;
}


//
// Material
//
Material::Material(unsigned int myShaderId, unsigned int albedoMap, unsigned int normalMap, unsigned int metallicMap, unsigned int roughnessMap, glm::vec4 offsetTiling)
{
	Material::myShaderId = myShaderId;
	Material::albedoMap = albedoMap;
	Material::normalMap = normalMap;
	Material::metallicMap = metallicMap;
	Material::roughnessMap = roughnessMap;
	Material::tilingAndOffset = offsetTiling;
}

void Material::applyTextureUniforms(const glm::mat4& modelMatrix)
{
	setupShader(myShaderId, &modelMatrix);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, albedoMap);
	glUniform1i(glGetUniformLocation(myShaderId, "albedoMap"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalMap);
	glUniform1i(glGetUniformLocation(myShaderId, "normalMap"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, metallicMap);
	glUniform1i(glGetUniformLocation(myShaderId, "metallicMap"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, roughnessMap);
	glUniform1i(glGetUniformLocation(myShaderId, "roughnessMap"), 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, MainLoop::getInstance().renderManager->getIrradianceMap());
	glUniform1i(glGetUniformLocation(myShaderId, "irradianceMap"), 4);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_CUBE_MAP, MainLoop::getInstance().renderManager->getPrefilterMap());
	glUniform1i(glGetUniformLocation(myShaderId, "prefilterMap"), 5);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, MainLoop::getInstance().renderManager->getBRDFLUTTexture());
	glUniform1i(glGetUniformLocation(myShaderId, "brdfLUT"), 6);

	glActiveTexture(GL_TEXTURE0);       // Reset before drawing mesh

	glUniform4fv(glGetUniformLocation(myShaderId, "tilingAndOffset"), 1, glm::value_ptr(tilingAndOffset));
}

float* Material::getTilingPtr()
{
	return &tilingAndOffset.x;
}

float* Material::getOffsetPtr()
{
	return &tilingAndOffset.z;
}

void Material::setTilingAndOffset(glm::vec4 tilingAndOffset)
{
	Material::tilingAndOffset = tilingAndOffset;
}


//
// ZellyMaterial
//
ZellyMaterial::ZellyMaterial(glm::vec3 color) :
	Material(*(unsigned int*)Resources::getResource("shader;zelly"), 0, 0, 0, 0, glm::vec4(0.0f))
{
	ZellyMaterial::color = color;
}

void ZellyMaterial::applyTextureUniforms(const glm::mat4& modelMatrix)
{
	setupShader(myShaderId, &modelMatrix);

	glUniform3fv(glGetUniformLocation(myShaderId, "zellyColor"), 1, &color.x);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, MainLoop::getInstance().renderManager->getIrradianceMap());
	glUniform1i(glGetUniformLocation(myShaderId, "irradianceMap"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, MainLoop::getInstance().renderManager->getPrefilterMap());
	glUniform1i(glGetUniformLocation(myShaderId, "prefilterMap"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, MainLoop::getInstance().renderManager->getBRDFLUTTexture());
	glUniform1i(glGetUniformLocation(myShaderId, "brdfLUT"), 2);
}
