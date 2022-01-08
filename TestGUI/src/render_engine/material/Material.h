#pragma once

#include <glm/glm.hpp>
#include <vector>


class Texture;

// NOTE: this implementation is simply a pbr standard shader-like implementation btw (metallic workflow)
class Material
{
public:
	Material(unsigned int myShaderId, Texture* albedoMap, Texture* normalMap, Texture* metallicMap, Texture* roughnessMap, float fadeAlpha = 1.0f, glm::vec4 offsetTiling = glm::vec4(1, 1, 0, 0));

	// TODO: would need to set the shader in this function to be used
	virtual void applyTextureUniforms();		// @Future: This'll be a virtual function that just gets called when needed. If there are different types of shaders that require different types of materials, then this'll use that different type (Maybe: it'll require that the shader func glUseProgram(id) will have to be taken care of in this material system too.... hmmmmmm
	virtual Texture* getMainTexture();			// NOTE: This is for Z-prepass and shadowmaps along with alphaCutoff

	float* getTilingPtr();
	float* getOffsetPtr();
	void setTilingAndOffset(glm::vec4 tilingAndOffset);
	unsigned int getShaderId() { return myShaderId; }

	static bool resetFlag;

	float fadeAlpha;
	bool isTransparent = false;

protected:
	Texture
		*albedoMap,
		*normalMap,
		*metallicMap,
		*roughnessMap;

	glm::vec4 tilingAndOffset;
	unsigned int myShaderId;
};


class ZellyMaterial : public Material
{
public:
	ZellyMaterial(glm::vec3 color);

	void applyTextureUniforms();
	Texture* getMainTexture();
	inline glm::vec3& getColor() { return color; }

private:
	glm::vec3 color;
};


class LvlGridMaterial : public Material
{
public:
	LvlGridMaterial(glm::vec3 color);

	void applyTextureUniforms();
	Texture* getMainTexture();
	void setColor(glm::vec3 color) { LvlGridMaterial::color = color; }

private:
	glm::vec3 color;
};