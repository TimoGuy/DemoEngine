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
	Material(Shader* myShader, float ditherAlpha, float fadeAlpha, bool isTransparent, bool backsideRender = false, bool ignoreDepth = false);

	virtual void applyTextureUniforms(nlohmann::json injection = nullptr) = 0;
	virtual Texture* getMainTexture() = 0;
	Shader* getShader() { return myShader; }

	float ditherAlpha;
	float fadeAlpha;
	bool isTransparent;
	bool backsideRender;
	bool ignoreDepth;

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
	ZellyMaterial(glm::vec3 color, glm::vec3 color2);

	void applyTextureUniforms(nlohmann::json injection = nullptr);
	Texture* getMainTexture();
	inline glm::vec3& getColor() { return color; }
	inline glm::vec3& getColor2() { return color2; }

private:
	glm::vec3 color, color2;
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


class StaminaMeterMaterial : public Material
{
public:
	static StaminaMeterMaterial& getInstance();
private:
	StaminaMeterMaterial();
public:
	void applyTextureUniforms(nlohmann::json injection = nullptr);
	Texture* getMainTexture();

private:
	// @Hardcoded
	glm::vec3 staminaBarColor1{ 0.0588, 0.0588, 0.0588 };
	glm::vec3 staminaBarColor2{ 0.3216, 0.7765, 0.3647 };
	glm::vec3 staminaBarColor3{ 0.1686, 0.4275, 0.1922 };
	glm::vec3 staminaBarColor4{ 0.5804, 0.05098, 0.05098 };
	float staminaBarDepleteColorIntensity = 1024.0f;		// Looks like a lightsaber
};


class BottledWaterBobbingMaterial : public Material
{
public:
	static BottledWaterBobbingMaterial& getInstance();
private:
	BottledWaterBobbingMaterial();
public:
	void applyTextureUniforms(nlohmann::json injection = nullptr);
	Texture* getMainTexture();

	void setTopAndBottomYWorldSpace(float topY, float bottomY);

private:
	glm::vec2 topAndBottomYWorldSpace;		// @NOTE: this value needs to get filled in before drawing. It should get calculated with the bounds of the bottle
};