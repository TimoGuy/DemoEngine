#include "Material.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include "../resources/Resources.h"
#include "../render_manager/RenderManager.h"
#include "../../mainloop/MainLoop.h"

#include <iostream>

bool Material::resetFlag = false;

//
// Helper funks
//
GLuint currentShaderId = 0;
glm::mat3 currentSunSpinAmount;
float mapInterpolationAmt = -1;
void setupShader(GLuint shaderId)
{
	//
	// Only update whatever is necessary (when shader changes, the uniforms need to be rewritten fyi)
	//
	bool changeAll = Material::resetFlag;			// NOTE: at the start of every frame (RenderManager: renderScene()), Material::resetFlag is set to true
	if (changeAll || currentShaderId != shaderId)
	{
		changeAll = true;

		glUseProgram(shaderId);
		glUniformMatrix4fv(glGetUniformLocation(shaderId, "cameraMatrix"), 1, GL_FALSE, glm::value_ptr(MainLoop::getInstance().camera.calculateProjectionMatrix() * MainLoop::getInstance().camera.calculateViewMatrix()));
		MainLoop::getInstance().renderManager->setupSceneLights(shaderId);			// tODO: I'd really like to move to UBO's so we don't have to do this anymore
		
		currentShaderId = shaderId;
	}

	const glm::mat3 sunSpinAmount = MainLoop::getInstance().renderManager->getSunSpinAmount();
	if (changeAll || currentSunSpinAmount != sunSpinAmount)
	{
		glUniformMatrix3fv(glGetUniformLocation(shaderId, "sunSpinAmount"), 1, GL_FALSE, glm::value_ptr(sunSpinAmount));
		currentSunSpinAmount = sunSpinAmount;
	}

	const float mapInterp= MainLoop::getInstance().renderManager->getMapInterpolationAmt();
	if (changeAll || mapInterpolationAmt != mapInterp)
	{
		glUniform1f(glGetUniformLocation(shaderId, "mapInterpolationAmt"), mapInterp);
		mapInterpolationAmt = mapInterp;
	}

	Material::resetFlag = false;
}


//
// Material
//
Material::Material(unsigned int myShaderId, unsigned int* albedoMap, unsigned int* normalMap, unsigned int* metallicMap, unsigned int* roughnessMap, float fadeAlpha, glm::vec4 offsetTiling)
{
	Material::myShaderId = myShaderId;
	Material::albedoMapPtr = albedoMap;
	Material::normalMapPtr = normalMap;
	Material::metallicMapPtr = metallicMap;
	Material::roughnessMapPtr = roughnessMap;
	Material::fadeAlpha = fadeAlpha;
	Material::tilingAndOffset = offsetTiling;

	isTransparent = (fadeAlpha != 1.0f);
}

void Material::applyTextureUniforms()
{
	setupShader(myShaderId);

	if (albedoMapPtr != nullptr)
		albedoMap = *albedoMapPtr;
	if (normalMapPtr != nullptr)
		normalMap = *normalMapPtr;
	if (metallicMapPtr != nullptr)
		metallicMap = *metallicMapPtr;
	if (roughnessMapPtr != nullptr)
		roughnessMap = *roughnessMapPtr;

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
	glBindTexture(GL_TEXTURE_CUBE_MAP, MainLoop::getInstance().renderManager->getIrradianceMap2());
	glUniform1i(glGetUniformLocation(myShaderId, "irradianceMap2"), 5);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_CUBE_MAP, MainLoop::getInstance().renderManager->getPrefilterMap());
	glUniform1i(glGetUniformLocation(myShaderId, "prefilterMap"), 6);

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_CUBE_MAP, MainLoop::getInstance().renderManager->getPrefilterMap2());
	glUniform1i(glGetUniformLocation(myShaderId, "prefilterMap2"), 7);

	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, MainLoop::getInstance().renderManager->getBRDFLUTTexture());
	glUniform1i(glGetUniformLocation(myShaderId, "brdfLUT"), 8);

	glActiveTexture(GL_TEXTURE0);       // Reset before drawing mesh

	glUniform1f(glGetUniformLocation(myShaderId, "fadeAlpha"), fadeAlpha);
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
	Material(*(unsigned int*)Resources::getResource("shader;zelly"), 0, 0, 0, 0, 0.0f, glm::vec4(0.0f))
{
	ZellyMaterial::color = color;
	isTransparent = true;
}

void ZellyMaterial::applyTextureUniforms()
{
	setupShader(myShaderId);

	glUniform3fv(glGetUniformLocation(myShaderId, "zellyColor"), 1, &color.x);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, MainLoop::getInstance().renderManager->getIrradianceMap());
	glUniform1i(glGetUniformLocation(myShaderId, "irradianceMap"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, MainLoop::getInstance().renderManager->getIrradianceMap2());
	glUniform1i(glGetUniformLocation(myShaderId, "irradianceMap2"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, MainLoop::getInstance().renderManager->getPrefilterMap());
	glUniform1i(glGetUniformLocation(myShaderId, "prefilterMap"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, MainLoop::getInstance().renderManager->getPrefilterMap2());
	glUniform1i(glGetUniformLocation(myShaderId, "prefilterMap2"), 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, MainLoop::getInstance().renderManager->getBRDFLUTTexture());
	glUniform1i(glGetUniformLocation(myShaderId, "brdfLUT"), 4);
}


//
// LvlGridMaterial
//
LvlGridMaterial::LvlGridMaterial(glm::vec3 color) :
	Material(*(unsigned int*)Resources::getResource("shader;lvlGrid"), 0, 0, 0, 0, 0.0f, glm::vec4(50, 50, 0, 0))
{
	LvlGridMaterial::color = color;
	isTransparent = true;
}

void LvlGridMaterial::applyTextureUniforms()
{
	setupShader(myShaderId);

	glUniform3fv(glGetUniformLocation(myShaderId, "gridColor"), 1, &color.x);
	glUniform4fv(glGetUniformLocation(myShaderId, "tilingAndOffset"), 1, glm::value_ptr(tilingAndOffset));

	GLuint gridTexture = *(GLuint*)Resources::getResource("texture;lvlGridTexture");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gridTexture);
	glUniform1i(glGetUniformLocation(myShaderId, "gridTexture"), 0);
}
