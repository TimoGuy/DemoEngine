#pragma once


// NOTE: this implementation is simply a pbr standard shader-like implementation btw (metallic workflow)
class Material
{
public:
	Material(unsigned int albedoMap, unsigned int normalMap, unsigned int metallicMap, unsigned int roughnessMap);

	void applyTextureUniforms(unsigned int shaderId, unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture);		// @Future: This'll be a virtual function that just gets called when needed. If there are different types of shaders that require different types of materials, then this'll use that different type (Maybe: it'll require that the shader func glUseProgram(id) will have to be taken care of in this material system too.... hmmmmmm

private:
	unsigned int
		albedoMap,
		normalMap,
		metallicMap,
		roughnessMap;
};