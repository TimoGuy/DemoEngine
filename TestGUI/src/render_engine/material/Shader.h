#pragma once

#include <map>
#include <string>
#include <vector>
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

	void setBool(std::string& uniformName, bool value);
	void setBool(std::string& uniformName, bool value);
	void setBool(std::string& uniformName, bool value);
	void setBool(std::string& uniformName, bool value);
	void setBool(std::string& uniformName, bool value);
	void setBool(std::string& uniformName, bool value);
	void setBool(std::string& uniformName, bool value);
	void setBool(std::string& uniformName, bool value);
	void setBool(std::string& uniformName, bool value);
	void setBool(std::string& uniformName, bool value);
	void setBool(std::string& uniformName, bool value);
	void setBool(std::string& uniformName, bool value);
	void setSampler(std::string& uniformName, bool value);

private:
	static UniformDataType strToDataType(const std::string& str);

	ShaderType type;
public:
	GLuint programId;
private:
	std::vector<ShaderUniform> props;
	std::vector<ShaderExt*> extensions;

	static Shader* currentlyBound;
};
