#include "Material.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include "Texture.h"
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
Material::Material(unsigned int myShaderId, Texture* albedoMap, Texture* normalMap, Texture* metallicMap, Texture* roughnessMap, float fadeAlpha, glm::vec4 offsetTiling)
{
	Material::myShaderId = myShaderId;
	Material::albedoMap = albedoMap;
	Material::normalMap = normalMap;
	Material::metallicMap = metallicMap;
	Material::roughnessMap = roughnessMap;
	Material::fadeAlpha = fadeAlpha;
	Material::tilingAndOffset = offsetTiling;

	isTransparent = (fadeAlpha != 1.0f);
}

void Material::applyTextureUniforms()
{
	setupShader(myShaderId);

	GLuint texUnit = 0;

	glBindTextureUnit(texUnit, albedoMap->getHandle());
	glUniform1i(glGetUniformLocation(myShaderId, "albedoMap"), texUnit);
	texUnit++;

	glBindTextureUnit(texUnit, normalMap->getHandle());
	glUniform1i(glGetUniformLocation(myShaderId, "normalMap"), texUnit);
	texUnit++;

	glBindTextureUnit(texUnit, metallicMap->getHandle());
	glUniform1i(glGetUniformLocation(myShaderId, "metallicMap"), texUnit);
	texUnit++;

	glBindTextureUnit(texUnit, roughnessMap->getHandle());
	glUniform1i(glGetUniformLocation(myShaderId, "roughnessMap"), texUnit);
	texUnit++;

	glBindTextureUnit(texUnit, MainLoop::getInstance().renderManager->getIrradianceMap());
	glUniform1i(glGetUniformLocation(myShaderId, "irradianceMap"), texUnit);
	texUnit++;

	glBindTextureUnit(texUnit, MainLoop::getInstance().renderManager->getIrradianceMap2());
	glUniform1i(glGetUniformLocation(myShaderId, "irradianceMap2"), texUnit);
	texUnit++;

	glBindTextureUnit(texUnit, MainLoop::getInstance().renderManager->getPrefilterMap());
	glUniform1i(glGetUniformLocation(myShaderId, "prefilterMap"), texUnit);
	texUnit++;

	glBindTextureUnit(texUnit, MainLoop::getInstance().renderManager->getPrefilterMap2());
	glUniform1i(glGetUniformLocation(myShaderId, "prefilterMap2"), texUnit);
	texUnit++;

	glBindTextureUnit(texUnit, MainLoop::getInstance().renderManager->getBRDFLUTTexture());
	glUniform1i(glGetUniformLocation(myShaderId, "brdfLUT"), texUnit);
	texUnit++;

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

	glBindTextureUnit(0, MainLoop::getInstance().renderManager->getIrradianceMap());
	glUniform1i(glGetUniformLocation(myShaderId, "irradianceMap"), 0);

	glBindTextureUnit(1, MainLoop::getInstance().renderManager->getIrradianceMap2());
	glUniform1i(glGetUniformLocation(myShaderId, "irradianceMap2"), 1);

	glBindTextureUnit(2, MainLoop::getInstance().renderManager->getPrefilterMap());
	glUniform1i(glGetUniformLocation(myShaderId, "prefilterMap"), 2);

	glBindTextureUnit(3, MainLoop::getInstance().renderManager->getPrefilterMap2());
	glUniform1i(glGetUniformLocation(myShaderId, "prefilterMap2"), 3);

	glBindTextureUnit(4, MainLoop::getInstance().renderManager->getBRDFLUTTexture());
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

	GLuint gridTexture = ((Texture*)Resources::getResource("texture;lvlGridTexture"))->getHandle();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gridTexture);
	glUniform1i(glGetUniformLocation(myShaderId, "gridTexture"), 0);
}
