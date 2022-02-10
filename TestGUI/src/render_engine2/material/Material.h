#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "../../utils/json.hpp"

typedef unsigned int GLuint;
class Texture;
class Shader;


class Material
{
public:
	Material(Shader* myShader, float ditherAlpha, float fadeAlpha, bool isTransparent);

	virtual void applyTextureUniforms(nlohmann::json injection = nullptr) = 0;
	virtual Texture* getMainTexture() = 0;
	Shader* getShader() { return myShader; }

	static bool resetFlag;

	float ditherAlpha;
	float fadeAlpha;
	bool isTransparent;

protected:
	Shader* myShader;
};


class PBRMaterial : public Material
{
public:
	PBRMaterial(
		Texture* albedoMap,
		Texture* normalMap,
		Texture* metallicMap,
		Texture* roughnessMap,
		float fadeAlpha = 1.0f,
		glm::vec4 offsetTiling = glm::vec4(1, 1, 0, 0)
	);

	// TODO: would need to set the shader in this function to be used
	virtual void applyTextureUniforms(nlohmann::json injection = nullptr);		// @Future: This'll be a virtual function that just gets called when needed. If there are different types of shaders that require different types of materials, then this'll use that different type (Maybe: it'll require that the shader func glUseProgram(id) will have to be taken care of in this material system too.... hmmmmmm
	virtual Texture* getMainTexture();			// NOTE: This is for Z-prepass and shadowmaps along with alphaCutoff

	inline void setTilingAndOffset(glm::vec4 tilingAndOffset) { PBRMaterial::tilingAndOffset = tilingAndOffset; }

private:
	Texture
		*albedoMap,
		*normalMap,
		*metallicMap,
		*roughnessMap;

	glm::vec4 tilingAndOffset;
};


class ZellyMaterial : public Material
{
public:
	ZellyMaterial(glm::vec3 color);

	void applyTextureUniforms(nlohmann::json injection = nullptr);
	Texture* getMainTexture();
	inline glm::vec3& getColor() { return color; }

private:
	glm::vec3 color;
};


class LvlGridMaterial : public Material
{
public:
	LvlGridMaterial(glm::vec3 color);

	void applyTextureUniforms(nlohmann::json injection = nullptr);
	Texture* getMainTexture();

	inline void setColor(glm::vec3 color) { LvlGridMaterial::color = color; }
	inline void setTilingAndOffset(glm::vec4 tilingAndOffset) { LvlGridMaterial::tilingAndOffset = tilingAndOffset; }

private:
	glm::vec3 color;
	glm::vec4 tilingAndOffset;
};