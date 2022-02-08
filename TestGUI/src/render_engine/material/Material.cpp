#include "Material.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include "Texture.h"
#include "../resources/Resources.h"
#include "../render_manager/RenderManager.h"
#include "../material/Shader.h"
#include "../../mainloop/MainLoop.h"

#include <iostream>


bool Material::resetFlag = false;

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
	myShader->setMat4("cameraMatrix", MainLoop::getInstance().camera.calculateProjectionMatrix() * MainLoop::getInstance().camera.calculateViewMatrix());
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
ZellyMaterial::ZellyMaterial(glm::vec3 color) :
	Material((Shader*)Resources::getResource("shader;zelly"), 1.0f, 0.0f, true)
{
	ZellyMaterial::color = color;
}

void ZellyMaterial::applyTextureUniforms(nlohmann::json injection)
{
	myShader->use();
	myShader->setMat4("cameraMatrix", MainLoop::getInstance().camera.calculateProjectionMatrix() * MainLoop::getInstance().camera.calculateViewMatrix());
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
	LvlGridMaterial::tilingAndOffset = glm::vec4(50, 50, 0, 0);
}

void LvlGridMaterial::applyTextureUniforms(nlohmann::json injection)
{
	myShader->use();
	myShader->setMat4("cameraMatrix", MainLoop::getInstance().camera.calculateProjectionMatrix() * MainLoop::getInstance().camera.calculateViewMatrix());
	myShader->setVec3("gridColor", color);
	myShader->setVec4("tilingAndOffset", tilingAndOffset);
	myShader->setSampler("gridTexture", ((Texture*)Resources::getResource("texture;lvlGridTexture"))->getHandle());
}

Texture* LvlGridMaterial::getMainTexture()
{
	return (Texture*)Resources::getResource("texture;lvlGridTexture");
}
