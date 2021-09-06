#include "ShaderResources.h"


unsigned int ShaderResources::setupShaderProgramVF(const std::string& programName, const std::string& vertFname, const std::string& fragFname)
{
	unsigned int programId = ShaderResources::getInstance().findShaderProgram(programName);
	if (!programId)
	{
		programId = glCreateProgram();
		unsigned int vShader = ShaderResources::compileShader(GL_VERTEX_SHADER, vertFname.c_str());
		unsigned int fShader = ShaderResources::compileShader(GL_FRAGMENT_SHADER, fragFname.c_str());
		glAttachShader(programId, vShader);
		glAttachShader(programId, fShader);
		glLinkProgram(programId);
		glDeleteShader(vShader);
		glDeleteShader(fShader);

		ShaderResources::getInstance().registerShaderProgram(programName, programId);
	}
	return programId;
}

unsigned int ShaderResources::findShaderProgram(const std::string& name)
{
	if (shaderProgramMap.find(name) != shaderProgramMap.end())
	{
		return shaderProgramMap[name];
	}

	return 0;
}

void ShaderResources::registerShaderProgram(const std::string& name, unsigned int programId)
{
	shaderProgramMap[name] = programId;
}

bool ShaderResources::eraseShaderProgram(const std::string& name)
{
	if (shaderProgramMap.find(name) != shaderProgramMap.end())
	{
		glDeleteProgram(shaderProgramMap[name]);
		shaderProgramMap.erase(name);
		return true;
	}
	return false;
}

unsigned int ShaderResources::compileShader(GLenum type, const char* fname)
{
	GLuint shader_id = glCreateShader(type);
	const char* filedata = readFile(fname);

	glShaderSource(shader_id, 1, &filedata, NULL);
	glCompileShader(shader_id);

	GLint compiled;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLsizei len;
		glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &len);

		GLchar* log = (GLchar*)malloc(sizeof(GLchar) * (len + 1));
		glGetShaderInfoLog(shader_id, len, &len, log);
		printf("[%s]\nShader compilation failed:\n%s\n", fname, log);
		free((GLchar*)log);
	}

	return shader_id;
}

const GLchar* ShaderResources::readFile(const char* filename)
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
