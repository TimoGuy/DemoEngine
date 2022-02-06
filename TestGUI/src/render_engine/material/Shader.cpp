#include "Shader.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <glad/glad.h>

#include "../../utils/json.hpp"
#include "../../utils/Utils.h"

const GLchar* readFile(const char* filename);
GLuint compileShader(nlohmann::json& params, GLenum type, std::string fname);


Shader::Shader(const std::string& fname)
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
	//
	if (params.contains("extensions"))
	{
		std::vector<std::string> _ext = params["extensions"];
		extensions = _ext;
	}

	std::cout << "HEY" << std::endl;
}


Shader::~Shader()
{
	glDeleteProgram(programId);
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
