#pragma once

#include <glm/glm.hpp>


// NOTE: this implementation is simply a pbr standard shader-like implementation btw (metallic workflow)
class Material
{
public:
	Material(unsigned int albedoMap, unsigned int normalMap, unsigned int metallicMap, unsigned int roughnessMap, glm::vec4 offsetTiling = glm::vec4(1, 1, 0, 0));

	void applyTextureUniforms(unsigned int shaderId, unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture);		// @Future: This'll be a virtual function that just gets called when needed. If there are different types of shaders that require different types of materials, then this'll use that different type (Maybe: it'll require that the shader func glUseProgram(id) will have to be taken care of in this material system too.... hmmmmmm
	float* getTilingPtr();
	float* getOffsetPtr();
	void setTilingAndOffset(glm::vec4 tilingAndOffset);

private:
	unsigned int
		albedoMap,
		normalMap,
		metallicMap,
		roughnessMap;

	glm::vec4 tilingAndOffset;
};