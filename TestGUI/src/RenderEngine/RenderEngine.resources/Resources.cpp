#include "Resources.h"

#include <iostream>
#include <vector>
#include <stb/stb_image.h>

#include "../RenderEngine.model/RenderEngine.model.animation/Animation.h"


void* findResource(const std::string& resourceName);
void registerResource(const std::string& resourceName, void* resource);
void unregisterResource(const std::string& resourceName);

const GLchar* readFile(const char* filename);
GLuint compileShader(GLenum type, const char* fname);
void* loadShaderProgramVF(const std::string& programName, bool isUnloading, const std::string& vertFname, const std::string& fragFname);
void* loadShaderProgramVGF(const std::string& programName, bool isUnloading, const std::string& vertFname, const std::string& geomFname, const std::string& fragFname);

void* loadTexture2D(const std::string& textureName, bool isUnloading, const std::string& fname, GLuint fromTexture, GLuint toTexture, GLuint minFilter, GLuint magFilter, GLuint wrapS, GLuint wrapT);
void* loadHDRTexture2D(const std::string& textureName, bool isUnloading, const std::string& fname, GLuint fromTexture, GLuint toTexture, GLuint minFilter, GLuint magFilter, GLuint wrapS, GLuint wrapT);

void* loadModel(const std::string& modelName, bool isUnloading, const char* path);
void* loadModel(const std::string& modelName, bool isUnloading, const char* path, std::vector<int> animationIndices);

void* loadResource(const std::string& resourceName, bool isUnloading);


namespace Resources
{
	std::map<std::string, void*> resourceMap;

	std::map<std::string, void*>& getResourceMap() { return resourceMap; }

	// ---------------------------------------
	//   The almighty getResource() Function
	// ---------------------------------------
	void* getResource(const std::string& resourceName)
	{
		void* resource = findResource(resourceName);
		if (resource != NULL)
			return resource;

		resource = loadResource(resourceName, false);
		registerResource(resourceName, resource);
		return resource;
	}

	void reloadResource(const std::string& resourceName)
	{
		// Undo resource
		loadResource(resourceName, true);
		unregisterResource(resourceName);

		// Re-fetch that resource!
		void* resource = loadResource(resourceName, false);
		registerResource(resourceName, resource);
	}
}


void* findResource(const std::string& resourceName)
{
	if (Resources::resourceMap.find(resourceName) != Resources::resourceMap.end())
	{
		return Resources::resourceMap[resourceName];
	}

	return NULL;
}


void registerResource(const std::string& resourceName, void* resource)
{
	Resources::resourceMap[resourceName] = resource;
}


void unregisterResource(const std::string& resourceName)
{
	Resources::resourceMap.erase(resourceName);
}


#pragma region Shader Program Utils

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


GLuint compileShader(GLenum type, const char* fname)
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


void* loadShaderProgramVF(const std::string& programName, bool isUnloading, const std::string& vertFname, const std::string& fragFname)
{
	if (!isUnloading)
	{
		GLuint programId = glCreateProgram();
		GLuint vShader = compileShader(GL_VERTEX_SHADER, vertFname.c_str());
		GLuint fShader = compileShader(GL_FRAGMENT_SHADER, fragFname.c_str());
		glAttachShader(programId, vShader);
		glAttachShader(programId, fShader);
		glLinkProgram(programId);
		glDeleteShader(vShader);
		glDeleteShader(fShader);

		GLuint* payload = new GLuint();
		*payload = programId;
		registerResource(programName, payload);
		return payload;
	}
	else
	{
		glDeleteProgram(*(GLuint*)findResource(programName));
		return (void*)0;
	}
}


void* loadShaderProgramVGF(const std::string& programName, bool isUnloading, const std::string& vertFname, const std::string& geomFname, const std::string& fragFname)
{
	GLuint programId = glCreateProgram();
	GLuint vShader = compileShader(GL_VERTEX_SHADER, vertFname.c_str());
	GLuint gShader = compileShader(GL_GEOMETRY_SHADER, geomFname.c_str());
	GLuint fShader = compileShader(GL_FRAGMENT_SHADER, fragFname.c_str());
	glAttachShader(programId, vShader);
	glAttachShader(programId, gShader);
	glAttachShader(programId, fShader);
	glLinkProgram(programId);
	glDeleteShader(vShader);
	glDeleteShader(gShader);
	glDeleteShader(fShader);

	GLuint* payload = new GLuint();
	*payload = programId;
	registerResource(programName, payload);
	return payload;
}

#pragma endregion


#pragma region Texture Resources Utils

void* loadTexture2D(
	const std::string& textureName,
	bool isUnloading,
	const std::string& fname,
	GLuint fromTexture = GL_RGB,
	GLuint toTexture = GL_RGB,
	GLuint minFilter = GL_NEAREST,
	GLuint magFilter = GL_NEAREST,
	GLuint wrapS = GL_REPEAT,
	GLuint wrapT = GL_REPEAT)
{
	stbi_set_flip_vertically_on_load(true);
	int imgWidth, imgHeight, numColorChannels;
	unsigned char* bytes = stbi_load(fname.c_str(), &imgWidth, &imgHeight, &numColorChannels, STBI_default);

	if (bytes == NULL)
	{
		std::cout << stbi_failure_reason() << std::endl;
	}

	GLuint textureId;
	glGenTextures(1, &textureId);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

	glTexImage2D(GL_TEXTURE_2D, 0, fromTexture, imgWidth, imgHeight, 0, toTexture, GL_UNSIGNED_BYTE, bytes);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(bytes);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLuint* payload = new GLuint();
	*payload = textureId;
	registerResource(textureName, payload);
	return payload;
}


void* loadHDRTexture2D(
	const std::string& textureName,
	bool isUnloading,
	const std::string& fname,
	GLuint fromTexture = GL_RGB16F,
	GLuint toTexture = GL_RGB,
	GLuint minFilter = GL_NEAREST,
	GLuint magFilter = GL_NEAREST,
	GLuint wrapS = GL_REPEAT,
	GLuint wrapT = GL_REPEAT)
{
	stbi_set_flip_vertically_on_load(true);
	int imgWidth, imgHeight, numColorChannels;
	float* bytes = stbi_loadf(fname.c_str(), &imgWidth, &imgHeight, &numColorChannels, STBI_default);

	if (bytes == NULL)
	{
		std::cout << stbi_failure_reason() << std::endl;
	}

	GLuint textureId;
	glGenTextures(1, &textureId);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

	glTexImage2D(GL_TEXTURE_2D, 0, fromTexture, imgWidth, imgHeight, 0, toTexture, GL_FLOAT, bytes);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(bytes);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLuint* payload = new GLuint();
	*payload = textureId;
	registerResource(textureName, payload);
	return payload;
}

#pragma endregion


#pragma region Model And Animation Loading Utils

void* loadModel(const std::string& modelName, bool isUnloading, const char* path)
{
	Model* model = new Model(path);
	registerResource(modelName, model);
	return model;
}

void* loadModel(const std::string& modelName, bool isUnloading, const char* path, std::vector<int> animationIndices)
{
	Model* model = new Model(path, animationIndices);
	registerResource(modelName, model);
	return model;
}

#pragma endregion


void* loadResource(const std::string& resourceName, bool isUnloading)
{
	//
	// NOTE: this is gonna be a huge function btw
	//
	if (resourceName == "shader;pbr")							return loadShaderProgramVF(resourceName, isUnloading, "pbr.vert", "pbr.frag");
	if (resourceName == "shader;blinnPhong")					return loadShaderProgramVF(resourceName, isUnloading, "vertex.vert", "fragment.frag");
	if (resourceName == "shader;blinnPhongSkinned")				return loadShaderProgramVF(resourceName, isUnloading, "model.vert", "model.frag");
	if (resourceName == "shader;skybox")						return loadShaderProgramVF(resourceName, isUnloading, "skybox.vert", "skybox.frag");
	if (resourceName == "shader;shadowPass")					return loadShaderProgramVF(resourceName, isUnloading, "shadow.vert", "do_nothing.frag");
	if (resourceName == "shader;csmShadowPass")					return loadShaderProgramVGF(resourceName, isUnloading, "csm_shadow.vert", "csm_shadow.geom", "do_nothing.frag");
	if (resourceName == "shader;debugCSM")						return loadShaderProgramVF(resourceName, isUnloading, "debug_csm.vert", "debug_csm.frag");
	if (resourceName == "shader;shadowPassSkinned")				return loadShaderProgramVF(resourceName, isUnloading, "shadow_skinned.vert", "do_nothing.frag");
	if (resourceName == "shader;text")							return loadShaderProgramVF(resourceName, isUnloading, "text.vert", "text.frag");
	if (resourceName == "shader;hdriGeneration")				return loadShaderProgramVF(resourceName, isUnloading, "cubemap.vert", "hdri_equirectangular.frag");
	if (resourceName == "shader;irradianceGeneration")			return loadShaderProgramVF(resourceName, isUnloading, "cubemap.vert", "irradiance_convolution.frag");
	if (resourceName == "shader;pbrPrefilterGeneration")		return loadShaderProgramVF(resourceName, isUnloading, "cubemap.vert", "prefilter.frag");
	if (resourceName == "shader;brdfGeneration")				return loadShaderProgramVF(resourceName, isUnloading, "brdf.vert", "brdf.frag");
	if (resourceName == "shader;bloom_postprocessing")			return loadShaderProgramVF(resourceName, isUnloading, "bloom_postprocessing.vert", "bloom_postprocessing.frag");
	if (resourceName == "shader;postprocessing")				return loadShaderProgramVF(resourceName, isUnloading, "postprocessing.vert", "postprocessing.frag");

	if (resourceName == "texture;pbrAlbedo")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/rustediron2_basecolor.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrNormal")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/rustediron2_normal.png", GL_RGB, GL_RGB, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrMetalness")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/rustediron2_metallic.png", GL_RED, GL_RED, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrRoughness")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/rustediron2_roughness.png", GL_RED, GL_RED, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;lightIcon")					return loadTexture2D(resourceName, isUnloading, "res/cool_img.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;hdrEnvironmentMap")			return loadHDRTexture2D(resourceName, isUnloading, "res/skybox/environment.hdr"/*"res/skybox/rice_field_day_env.hdr"*/, GL_RGB16F, GL_RGB, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

	if (resourceName == "model;slimeGirl")						return loadModel(resourceName, isUnloading, "res/slime_glb.glb", { 0, 1, 2, 3, 4, 5 });
	if (resourceName == "model;yosemiteTerrain")				return loadModel(resourceName, isUnloading, "res/cube.glb");
}
