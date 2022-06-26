#pragma once

#include <map>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "shaderext/ShaderExt.h"
typedef unsigned int GLuint;


// @SHADERPALETTE
enum class ShaderType { UNDEFINED, VF, VGF, C };
enum class UniformDataType
{
	BOOL,
	INT,
	UINT,
	FLOAT,
	VEC2,
	VEC3,
	VEC4,
	IVEC4,
	MAT3,
	MAT4,
	SAMPLER2D,
	SAMPLER2DARRAY,
	SAMPLERCUBE
};


struct ShaderUniform
{
	UniformDataType dataType;
	std::string name;
	bool isArray;  // NOTE: unfortunately there won't be functionality of how many items are in the array, since that's a lot of work  -Timo
};


class Shader
{
public:
	Shader(const std::string& fname);
	~Shader();

	void use();

	void setBool(std::string uniformName, const bool& value);
	void setInt(std::string uniformName, const int& value);
	void setUint(std::string uniformName, const unsigned int& value);
	void setFloat(std::string uniformName, const float& value);
	void setVec2(std::string uniformName, const glm::vec2& value);
	void setVec3(std::string uniformName, const glm::vec3& value);
	void setVec4(std::string uniformName, const glm::vec4& value);
	void setIvec4(std::string uniformName, const glm::ivec4& value);
	void setMat3(std::string uniformName, const glm::mat3& value);
	void setMat4(std::string uniformName, const glm::mat4& value);
	void setSampler(std::string uniformName, const GLuint& value);
	void resetSamplers();

private:
	static UniformDataType strToDataType(const std::string& str);

	ShaderType type;
public:
	GLuint programId;
private:
	std::map<std::string, int> uniformLocationCache;
	bool uniformLocationCacheCreated;
	int currentTexIndex;
	std::vector<ShaderUniform> props;
	std::vector<ShaderExt*> extensions;

	static Shader* currentlyBound;
};
