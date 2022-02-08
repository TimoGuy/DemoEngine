#include "Shader.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include "../../utils/json.hpp"
#include "../../utils/Utils.h"
#include "shaderext/ShaderExtZBuffer.h"
#include "shaderext/ShaderExtPBR_daynight_cycle.h"
#include "shaderext/ShaderExtShadow.h"
#include "shaderext/ShaderExtCSM_shadow.h"
#include "shaderext/ShaderExtSSAO.h"

const GLchar* readFile(const char* filename);
GLuint compileShader(nlohmann::json& params, GLenum type, std::string fname);

Shader* Shader::currentlyBound = nullptr;


Shader::Shader(const std::string& fname) : type(ShaderType::UNDEFINED), currentTexIndex(0)
{
	std::string fullFname = "shader/" + fname + ".json";
	std::ifstream i(fullFname);
	nlohmann::json params;
	i >> params;

	// Type of shader
	std::string t = params["type"];
	if		(t == "VF")     type = ShaderType::VF;
	else if (t == "VGF")    type = ShaderType::VGF;
	else if (t == "C")      type = ShaderType::C;
	else                    assert(0);

	//
	// Compile program
	//
	programId = glCreateProgram();
	std::string shaderFname;
	std::vector<GLuint> compiledShaders;
	if (type == ShaderType::C)
	{
		shaderFname = params["C"];
		GLuint c = compileShader(params, GL_COMPUTE_SHADER, shaderFname.c_str());
		glAttachShader(programId, c);
		compiledShaders.push_back(c);
	}
	else
	{
		shaderFname = params["V"];
		GLuint v = compileShader(params, GL_VERTEX_SHADER, shaderFname.c_str());
		glAttachShader(programId, v);
		compiledShaders.push_back(v);

		if (type == ShaderType::VGF)
		{
			shaderFname = params["G"];
			GLuint g = compileShader(params, GL_GEOMETRY_SHADER, shaderFname.c_str());
			glAttachShader(programId, g);
			compiledShaders.push_back(g);
		}

		shaderFname = params["F"];
		GLuint f = compileShader(params, GL_FRAGMENT_SHADER, shaderFname.c_str());
		glAttachShader(programId, f);
		compiledShaders.push_back(f);
	}

	// Too's Magic of Love
	glLinkProgram(programId);

	// Delete shaders
	for (size_t i = 0; i < compiledShaders.size(); i++)
		glAttachShader(programId, compiledShaders[i]);

	//
	// Load in all props
	//
	std::vector<std::string> propsStrs = params["props"];
	for (size_t i = 0; i < propsStrs.size(); i++)
	{
		ShaderUniform u;

		size_t splitIndex = propsStrs[i].find_first_of(' ');
		u.dataType = strToDataType(trimString(propsStrs[i].substr(0, splitIndex)));
		u.name = trimString(propsStrs[i].substr(splitIndex));
		u.isArray = (u.name.find("[") != std::string::npos);

		props.push_back(u);
	}

	//
	// Load in all extensions
	// @SHADERPALETTE
	//
	if (params.contains("extensions"))
	{
		std::vector<std::string> _ext = params["extensions"];
		for (size_t i = 0; i < _ext.size(); i++)
		{
			std::string& e = _ext[i];
			if (e == "zBuffer")				extensions.push_back(new ShaderExtZBuffer(this));
			if (e == "ssao")				extensions.push_back(new ShaderExtSSAO(this));
			if (e == "pbr_daynight_cycle")	extensions.push_back(new ShaderExtPBR_daynight_cycle(this));
			if (e == "shadow")				extensions.push_back(new ShaderExtShadow(this));
			if (e == "csm_shadow")			extensions.push_back(new ShaderExtCSM_shadow(this));
		}
	}
}


Shader::~Shader()
{
	glDeleteProgram(programId);
}


void Shader::use()
{
	currentTexIndex = 1;	// New shader means new textures, so we need to reset the texture binding counter
	for (size_t i = 0; i < extensions.size(); i++)
		extensions[i]->setupExtension();

	if (currentlyBound == this)
		return;

	glUseProgram(programId);
	currentlyBound = this;

}


void Shader::setBool(std::string uniformName, const bool& value) { setInt(uniformName, (int)value); }
void Shader::setInt(std::string uniformName, const int& value)
{
	//if (uniformLocationCache.find(uniformName) == uniformLocationCache.end())
		int hickery/*uniformLocationCache[uniformName]*/ = glGetUniformLocation(programId, uniformName.c_str());

	glProgramUniform1i(programId, hickery/*uniformLocationCache[uniformName]*/, value);
}


void Shader::setUint(std::string uniformName, const unsigned int& value)
{
	//if (uniformLocationCache.find(uniformName) == uniformLocationCache.end())
		int hickery/*uniformLocationCache[uniformName]*/ = glGetUniformLocation(programId, uniformName.c_str());

	glProgramUniform1ui(programId, hickery/*uniformLocationCache[uniformName]*/, value);
}


void Shader::setFloat(std::string uniformName, const float& value)
{
	//if (uniformLocationCache.find(uniformName) == uniformLocationCache.end())
		int hickery/*uniformLocationCache[uniformName]*/ = glGetUniformLocation(programId, uniformName.c_str());

	glProgramUniform1f(programId, hickery/*uniformLocationCache[uniformName]*/, value);
}


void Shader::setVec2(std::string uniformName, const glm::vec2& value)
{
	//if (uniformLocationCache.find(uniformName) == uniformLocationCache.end())
		int hickery/*uniformLocationCache[uniformName]*/ = glGetUniformLocation(programId, uniformName.c_str());

	glProgramUniform2fv(programId, hickery/*uniformLocationCache[uniformName]*/, 1, glm::value_ptr(value));
}


void Shader::setVec3(std::string uniformName, const glm::vec3& value)
{
	//if (uniformLocationCache.find(uniformName) == uniformLocationCache.end())
		int hickery/*uniformLocationCache[uniformName]*/ = glGetUniformLocation(programId, uniformName.c_str());

	glProgramUniform3fv(programId, hickery/*uniformLocationCache[uniformName]*/, 1, glm::value_ptr(value));
}


void Shader::setVec4(std::string uniformName, const glm::vec4& value)
{
	//if (uniformLocationCache.find(uniformName) == uniformLocationCache.end())
		int hickery/*uniformLocationCache[uniformName]*/ = glGetUniformLocation(programId, uniformName.c_str());

	glProgramUniform4fv(programId, hickery/*uniformLocationCache[uniformName]*/, 1, glm::value_ptr(value));
}


void Shader::setIvec4(std::string uniformName, const glm::ivec4& value)
{
	//if (uniformLocationCache.find(uniformName) == uniformLocationCache.end())
		int hickery/*uniformLocationCache[uniformName]*/ = glGetUniformLocation(programId, uniformName.c_str());

	glProgramUniform4iv(programId, hickery/*uniformLocationCache[uniformName]*/, 1, glm::value_ptr(value));
}


void Shader::setMat3(std::string uniformName, const glm::mat3& value)
{
	//if (uniformLocationCache.find(uniformName) == uniformLocationCache.end())
		int hickery/*uniformLocationCache[uniformName]*/ = glGetUniformLocation(programId, uniformName.c_str());

	glProgramUniformMatrix3fv(programId, hickery/*uniformLocationCache[uniformName]*/, 1, GL_FALSE, glm::value_ptr(value));
}


void Shader::setMat4(std::string uniformName, const glm::mat4& value)
{
	//if (uniformLocationCache.find(uniformName) == uniformLocationCache.end())
		int hickery/*uniformLocationCache[uniformName]*/ = glGetUniformLocation(programId, uniformName.c_str());

	glProgramUniformMatrix4fv(programId, hickery/*uniformLocationCache[uniformName]*/, 1, GL_FALSE, glm::value_ptr(value));
}


void Shader::setSampler(std::string uniformName, const GLuint& value)
{
	//if (uniformLocationCache.find(uniformName) == uniformLocationCache.end())
		int hickery/*uniformLocationCache[uniformName]*/ = glGetUniformLocation(programId, uniformName.c_str());

	glBindTextureUnit(currentTexIndex, value);
	glProgramUniform1i(programId, hickery/*uniformLocationCache[uniformName]*/, currentTexIndex);
	currentTexIndex++;
}


UniformDataType Shader::strToDataType(const std::string& str)
{
	if (str == "bool")				return UniformDataType::BOOL;
	if (str == "int")				return UniformDataType::INT;
	if (str == "uint")				return UniformDataType::UINT;
	if (str == "float")				return UniformDataType::FLOAT;
	if (str == "vec2")				return UniformDataType::VEC2;
	if (str == "vec3")				return UniformDataType::VEC3;
	if (str == "vec4")				return UniformDataType::VEC4;
	if (str == "ivec4")				return UniformDataType::IVEC4;
	if (str == "mat3")				return UniformDataType::MAT3;
	if (str == "mat4")				return UniformDataType::MAT4;
	if (str == "sampler2D")			return UniformDataType::SAMPLER2D;
	if (str == "sampler2DArray")	return UniformDataType::SAMPLER2DARRAY;
	if (str == "samplerCube")		return UniformDataType::SAMPLERCUBE;

	std::cout << "ERROR: the data type did not match up with any of the ones in here... " << str << std::endl;
	assert(0);
}


const GLchar* readFile(const char* filename)
{
	FILE* infile;

	fopen_s(&infile, filename, "rb");
	if (!infile) {
		printf("Unable to open file %s\n", filename);
		return NULL;
	}

	fseek(infile, 0, SEEK_END);
	int len = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	GLchar* source = (GLchar*)malloc(sizeof(GLchar) * (len + 1));
	fread(source, 1, len, infile);
	fclose(infile);
	source[len] = 0;

	return (GLchar*)(source);
}


GLuint compileShader(nlohmann::json& params, GLenum type, std::string fname)
{
	std::cout << std::left << std::setw(50) << ("[" + std::string(fname) + "]") << "Compiling Shader...\t";
	GLuint shader_id = glCreateShader(type);
	const GLchar* source = readFile(("shader/src/" + fname).c_str());

	glShaderSource(shader_id, 1, &source, NULL);
	glCompileShader(shader_id);

	GLint compiled;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		GLsizei len;
		glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &len);

		GLchar* log = (GLchar*)malloc(sizeof(GLchar) * (len + 1));
		glGetShaderInfoLog(shader_id, len, &len, log);
		printf("FAIL\n%s\n", log);
		free((GLchar*)log);
	}
	else
	{
		std::cout << "SUCCESS" << std::endl;
	}

	return shader_id;
}
