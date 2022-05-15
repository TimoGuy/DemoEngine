#include "Material.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include "Texture.h"
#include "../resources/Resources.h"
#include "../render_manager/RenderManager.h"
#include "../material/Shader.h"
#include "../../mainloop/MainLoop.h"
#include "../../utils/GameState.h"


Material::Material(Shader* myShader, float ditherAlpha, float fadeAlpha, bool isTransparent) : myShader(myShader), ditherAlpha(ditherAlpha), fadeAlpha(fadeAlpha), isTransparent(isTransparent) {}


//
// PBR Material
//
PBRMaterial::PBRMaterial(
	Texture* albedoMap,
	Texture* normalMap,
	Texture* metallicMap,
	Texture* roughnessMap,
	float fadeAlpha,
	glm::vec4 offsetTiling) :
		Material((Shader*)Resources::getResource("shader;pbr"), 1.0f, fadeAlpha, (fadeAlpha != 1.0f))
{
	PBRMaterial::albedoMap = albedoMap;
	PBRMaterial::normalMap = normalMap;
	PBRMaterial::metallicMap = metallicMap;
	PBRMaterial::roughnessMap = roughnessMap;
	PBRMaterial::tilingAndOffset = offsetTiling;
}

void PBRMaterial::applyTextureUniforms(nlohmann::json injection)
{
	myShader->use();
	myShader->setSampler("albedoMap", albedoMap->getHandle());
	myShader->setSampler("normalMap", normalMap->getHandle());
	myShader->setSampler("metallicMap", metallicMap->getHandle());
	myShader->setSampler("roughnessMap", roughnessMap->getHandle());

	myShader->setFloat("ditherAlpha", ditherAlpha);
	myShader->setFloat("fadeAlpha", fadeAlpha);
	myShader->setVec4("tilingAndOffset", tilingAndOffset);

	glm::vec3 color(1.0f);
	if (injection != nullptr && injection.contains("color"))
		color = { injection["color"][0], injection["color"][1], injection["color"][2] };

	myShader->setVec3("color", color);
}

Texture* PBRMaterial::getMainTexture()
{
	return albedoMap;
}


//
// ZellyMaterial
//
// @NOTE: The zelly material is opaque now bc of a decision I made with all the issues with the zelly material.
// Turns out that it'd be easier to do a subsurface scattering thing, bc look at the lack of transparency with this:
// https://www.youtube.com/shorts/dDZ9Mbf1GNo. Seems like by the time the faked subsurface scattering should be transparent,
// fresnel has already taken over and it's a reflection instead. Dang nabbit.
// 		-Timo
//
ZellyMaterial::ZellyMaterial(glm::vec3 color) :
	Material((Shader*)Resources::getResource("shader;zelly"), 1.0f, 1.0f, false)
{
	ZellyMaterial::color = color;
}

void ZellyMaterial::applyTextureUniforms(nlohmann::json injection)
{
	myShader->use();
	myShader->setVec3("zellyColor", color);
	myShader->setFloat("ditherAlpha", ditherAlpha);
}

Texture* ZellyMaterial::getMainTexture()
{
	return nullptr;
}


//
// LvlGridMaterial
//
LvlGridMaterial::LvlGridMaterial(glm::vec3 color) :
	Material((Shader*)Resources::getResource("shader;lvlGrid"), 1.0f, 0.0f, true)
{
	LvlGridMaterial::color = color;
	LvlGridMaterial::tilingAndOffset = glm::vec4(25, 25, 0, 0);
}

void LvlGridMaterial::applyTextureUniforms(nlohmann::json injection)
{
	myShader->use();
	myShader->setVec3("gridColor", color);
	myShader->setVec4("tilingAndOffset", tilingAndOffset);
	myShader->setSampler("gridTexture", ((Texture*)Resources::getResource("texture;lvlGridTexture"))->getHandle());
}

Texture* LvlGridMaterial::getMainTexture()
{
	return (Texture*)Resources::getResource("texture;lvlGridTexture");
}


//
// StaminaMeterMaterial
//
StaminaMeterMaterial& StaminaMeterMaterial::getInstance()
{
	static StaminaMeterMaterial instance;
	return instance;
}

StaminaMeterMaterial::StaminaMeterMaterial() :
	Material((Shader*)Resources::getResource("shader;hudUI"), 1.0f, 0.0f, true)
{
}

void StaminaMeterMaterial::applyTextureUniforms(nlohmann::json injection)
{
	float staminaAmountFilled = (float)GameState::getInstance().currentPlayerStaminaAmount / (float)GameState::getInstance().maxPlayerStaminaAmount;
	const float staminaDepleteChaser = (float)GameState::getInstance().playerStaminaDepleteChaser / (float)GameState::getInstance().maxPlayerStaminaAmount;
	staminaAmountFilled = glm::min(staminaAmountFilled, staminaDepleteChaser);		// @NOTE: This just creates a little animation as the stamina bar refills. It's just for effect.  -Timo

	myShader->use();

	constexpr size_t staminaWheelCount = 16;
	for (size_t i = 0; i < staminaWheelCount; i++)
	{
		myShader->setFloat("staminaAmountFilled[" + std::to_string(i) + "]", glm::clamp(staminaAmountFilled * (float)staminaWheelCount - (float)i, 0.0f, 1.0f));
		myShader->setFloat("staminaDepleteChaser[" + std::to_string(i) + "]", glm::clamp(staminaDepleteChaser * (float)staminaWheelCount - (float)i, 0.0f, 1.0f));
	}
	myShader->setVec3("staminaBarColor1", staminaBarColor1);
	myShader->setVec3("staminaBarColor2", staminaBarColor2);
	myShader->setVec3("staminaBarColor3", staminaBarColor3);
	myShader->setVec3("staminaBarColor4", staminaBarColor4);
	myShader->setFloat("staminaBarDepleteColorIntensity", staminaBarDepleteColorIntensity);
	myShader->setFloat("ditherAlpha", ditherAlpha);
	//myShader->setMat4("modelMatrix", modelMatrix);
}

Texture* StaminaMeterMaterial::getMainTexture()
{
	return nullptr;
}
