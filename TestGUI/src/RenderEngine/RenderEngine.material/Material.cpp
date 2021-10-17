#include "Material.h"

#include <glad/glad.h>


Material::Material(unsigned int albedoMap, unsigned int normalMap, unsigned int metallicMap, unsigned int roughnessMap)
{
	Material::albedoMap = albedoMap;
	Material::normalMap = normalMap;
	Material::metallicMap = metallicMap;
	Material::roughnessMap = roughnessMap;
}

void Material::applyTextureUniforms(unsigned int shaderId, unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture)
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, albedoMap);
	glUniform1i(glGetUniformLocation(shaderId, "albedoMap"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalMap);
	glUniform1i(glGetUniformLocation(shaderId, "normalMap"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, metallicMap);
	glUniform1i(glGetUniformLocation(shaderId, "metallicMap"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, roughnessMap);
	glUniform1i(glGetUniformLocation(shaderId, "roughnessMap"), 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	glUniform1i(glGetUniformLocation(shaderId, "irradianceMap"), 4);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	glUniform1i(glGetUniformLocation(shaderId, "prefilterMap"), 5);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
	glUniform1i(glGetUniformLocation(shaderId, "brdfLUT"), 6);

	glActiveTexture(GL_TEXTURE0);       // Reset before drawing mesh
}
