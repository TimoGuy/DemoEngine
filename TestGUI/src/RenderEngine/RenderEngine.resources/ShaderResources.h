#pragma once

#include <string>
#include <map>
#include <glad/glad.h>


class ShaderResources
{
public:
	static ShaderResources& getInstance()
	{
		static ShaderResources instance;
		return instance;
	}

	unsigned int setupShaderProgramVF(const std::string& programName, const std::string& vertFname, const std::string& fragFname);
	bool eraseShaderProgram(const std::string& name);
	static unsigned int compileShader(GLenum type, const char* fname);			// TODO: make this be in the private section

private:
	//
	// Singleton pattern
	//
	ShaderResources() {}
	//ShaderResources(ShaderResources const&);		// NOTE: do not implement!
	//void operator=(ShaderResources const&);			// NOTE: do not implement!

	std::map<std::string, unsigned int> shaderProgramMap;

	static const GLchar* readFile(const char* filename);
	unsigned int findShaderProgram(const std::string& name);
	void registerShaderProgram(const std::string& name, unsigned int programId);

public:
	/*ShaderResources(ShaderResources const&) = delete;
	void operator=(ShaderResources const&) = delete;*/
	// Note: Scott Meyers mentions in his Effective Modern
	//       C++ book, that deleted functions should generally
	//       be public as it results in better error messages
	//       due to the compilers behavior to check accessibility
	//       before deleted status
};
