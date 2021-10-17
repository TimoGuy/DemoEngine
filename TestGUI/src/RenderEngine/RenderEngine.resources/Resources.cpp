#include "Resources.h"

#include <iostream>
#include <iomanip>
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

void* loadPBRMaterial(const std::string& materialName, bool isUnloading, const std::string& albedoName, const std::string& normalName, const std::string& metallicName, const std::string& roughnessName);

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
	std::cout << std::left << std::setw(30) << ("[" + std::string(fname) + "]") << "Compiling Shader...\t";
	GLuint shader_id = glCreateShader(type);
	const char* filedata = readFile(fname);

	glShaderSource(shader_id, 1, &filedata, NULL);
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
		return payload;
	}
	else
	{
		glDeleteProgram(*(GLuint*)findResource(programName));
		return nullptr;
	}
}


void* loadShaderProgramVGF(const std::string& programName, bool isUnloading, const std::string& vertFname, const std::string& geomFname, const std::string& fragFname)
{
	if (!isUnloading)
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
		return payload;
	}
	else
	{
		glDeleteProgram(*(GLuint*)findResource(programName));
		return nullptr;
	}
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
	if (!isUnloading)
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
		return payload;
	}
	else
	{
		glDeleteTextures(1, (GLuint*)findResource(textureName));
		return nullptr;
	}
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
	if (!isUnloading)
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
		return payload;
	}
	else
	{
		glDeleteTextures(1, (GLuint*)findResource(textureName));
		return nullptr;
	}
}

#pragma endregion


#pragma region Model And Animation Loading Utils

void* loadModel(const std::string& modelName, bool isUnloading, const char* path)
{
	if (!isUnloading)
	{
		Model* model = new Model(path);
		return model;
	}
	else
	{
		delete (Model*)findResource(modelName);
		return nullptr;
	}
}

void* loadModel(const std::string& modelName, bool isUnloading, const char* path, std::vector<int> animationIndices)
{
	if (!isUnloading)
	{
		Model* model = new Model(path, animationIndices);
		return model;
	}
	else
	{
		delete (Model*)findResource(modelName);
		return nullptr;
	}
}

void* loadPBRMaterial(const std::string& materialName, bool isUnloading, const std::string& albedoName, const std::string& normalName, const std::string& metallicName, const std::string& roughnessName)
{
	if (!isUnloading)
	{
		Material* material =
			new Material(
				*(GLuint*)Resources::getResource(albedoName),
				*(GLuint*)Resources::getResource(normalName),
				*(GLuint*)Resources::getResource(metallicName),
				*(GLuint*)Resources::getResource(roughnessName)
			);
		return material;
	}
	else
	{
		delete (Material*)findResource(materialName);
		return nullptr;
	}
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
	if (resourceName == "shader;pointLightShadowPass")			return loadShaderProgramVGF(resourceName, isUnloading, "point_shadow.vert", "point_shadow.geom", "point_shadow.frag");
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

	if (resourceName == "model;slimeGirl")						return loadModel(resourceName, isUnloading, /*"res/slime_glb.glb"*/"res/slime_upgraded.glb", { /*0, 1, 2, 3, 4,*/ 5, 8});		// 5: idle; 8: running
	if (resourceName == "model;yosemiteTerrain")				return loadModel(resourceName, isUnloading, "res/cube.glb");
	if (resourceName == "model;houseInterior")					return loadModel(resourceName, isUnloading, "res/house_w_interior.glb");

	if (resourceName == "material;pbrRustyMetal")				return loadPBRMaterial(resourceName, isUnloading, "texture;pbrAlbedo", "texture;pbrNormal", "texture;pbrMetalness", "texture;pbrRoughness");

	// TODO: continue back here!~!!!!
	if (resourceName == "material;pbrSlimeBelt")				return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeBeltAlbedo", "texture;pbrSlimeBeltNormal", "texture;pbrSlimeBeltMetalness", "texture;pbrSlimeBeltRoughness");
	if (resourceName == "texture;pbrSlimeBeltAlbedo")			return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/1.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeBeltAccent")			return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeBeltAccentAlbedo", "texture;pbrSlimeBeltAccentNormal", "texture;pbrSlimeBeltAccentMetalness", "texture;pbrSlimeBeltAccentRoughness");
	if (resourceName == "texture;pbrAlbedo2")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/2.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeBody")				return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeBodyAlbedo", "texture;pbrSlimeBodyNormal", "texture;pbrSlimeBodyMetalness", "texture;pbrSlimeBodyRoughness");
	if (resourceName == "texture;pbrAlbedo3")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/3.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeEyebrow")				return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeEyebrowAlbedo", "texture;pbrSlimeEyebrowNormal", "texture;pbrSlimeEyebrowMetalness", "texture;pbrSlimeEyebrowRoughness");
	if (resourceName == "texture;pbrAlbedo4")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/4.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeEye")					return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeEyeAlbedo", "texture;pbrSlimeEyeNormal", "texture;pbrSlimeEyeMetalness", "texture;pbrSlimeEyeRoughness");
	if (resourceName == "texture;pbrAlbedo5")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/5.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeHair")				return loadPBRMaterial(resourceName, isUnloading, "texture;pbrAlbedo6", "texture;pbrNormal", "texture;pbrMetalness", "texture;pbrRoughness");
	if (resourceName == "texture;pbrAlbedo6")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/6.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeInvisible")			return loadPBRMaterial(resourceName, isUnloading, "texture;pbrAlbedo7", "texture;pbrNormal", "texture;pbrMetalness", "texture;pbrRoughness");
	if (resourceName == "texture;pbrAlbedo7")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/7.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeShoeAccent")			return loadPBRMaterial(resourceName, isUnloading, "texture;pbrAlbedo8", "texture;pbrNormal", "texture;pbrMetalness", "texture;pbrRoughness");
	if (resourceName == "texture;pbrAlbedo8")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/8.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeShoeBlack")			return loadPBRMaterial(resourceName, isUnloading, "texture;pbrAlbedo9", "texture;pbrNormal", "texture;pbrMetalness", "texture;pbrRoughness");
	if (resourceName == "texture;pbrAlbedo9")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/9.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeShoeWhite")			return loadPBRMaterial(resourceName, isUnloading, "texture;pbrAlbedo10", "texture;pbrNormal", "texture;pbrMetalness", "texture;pbrRoughness");
	if (resourceName == "texture;pbrAlbedo10")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/10.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeShoeWhite2")			return loadPBRMaterial(resourceName, isUnloading, "texture;pbrAlbedo11", "texture;pbrNormal", "texture;pbrMetalness", "texture;pbrRoughness");
	if (resourceName == "texture;pbrAlbedo11")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/11.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeShorts")				return loadPBRMaterial(resourceName, isUnloading, "texture;pbrAlbedo12", "texture;pbrNormal", "texture;pbrMetalness", "texture;pbrRoughness");
	if (resourceName == "texture;pbrAlbedo12")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/12.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeShortsAccent")		return loadPBRMaterial(resourceName, isUnloading, "texture;pbrAlbedo13", "texture;pbrNormal", "texture;pbrMetalness", "texture;pbrRoughness");
	if (resourceName == "texture;pbrAlbedo13")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/13.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeSweater")				return loadPBRMaterial(resourceName, isUnloading, "texture;pbrAlbedo14", "texture;pbrNormal", "texture;pbrMetalness", "texture;pbrRoughness");
	if (resourceName == "texture;pbrAlbedo14")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/14.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeSweater2")			return loadPBRMaterial(resourceName, isUnloading, "texture;pbrAlbedo15", "texture;pbrNormal", "texture;pbrMetalness", "texture;pbrRoughness");
	if (resourceName == "texture;pbrAlbedo15")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/15.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeTights")				return loadPBRMaterial(resourceName, isUnloading, "texture;pbrAlbedo16", "texture;pbrNormal", "texture;pbrMetalness", "texture;pbrRoughness");
	if (resourceName == "texture;pbrAlbedo16")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/16.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeUndershirt")			return loadPBRMaterial(resourceName, isUnloading, "texture;pbrAlbedo17", "texture;pbrNormal", "texture;pbrMetalness", "texture;pbrRoughness");
	if (resourceName == "texture;pbrAlbedo17")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/17.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeVest")				return loadPBRMaterial(resourceName, isUnloading, "texture;pbrAlbedo18", "texture;pbrNormal", "texture;pbrMetalness", "texture;pbrRoughness");
	if (resourceName == "texture;pbrAlbedo18")					return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/18.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	assert(false);
	return nullptr;
}
