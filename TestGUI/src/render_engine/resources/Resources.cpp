#include "Resources.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <glad/glad.h>
#include <stb/stb_image.h>

#include "../model/animation/Animation.h"


void* findResource(const std::string& resourceName);
void registerResource(const std::string& resourceName, void* resource);
void unregisterResource(const std::string& resourceName);

const GLchar* readFile(const char* filename);
GLuint compileShader(GLenum type, const char* fname);
void* loadShaderProgramVF(const std::string& programName, bool isUnloading, const std::string& vertFname, const std::string& fragFname);
void* loadShaderProgramVGF(const std::string& programName, bool isUnloading, const std::string& vertFname, const std::string& geomFname, const std::string& fragFname);

void* loadTexture2D(const std::string& textureName, bool isUnloading, const std::string& fname, GLuint toTexture, GLuint minFilter, GLuint magFilter, GLuint wrapS, GLuint wrapT, bool flipVertical);
void* loadHDRTexture2D(const std::string& textureName, bool isUnloading, const std::string& fname, GLuint fromTexture, GLuint toTexture, GLuint minFilter, GLuint magFilter, GLuint wrapS, GLuint wrapT, bool flipVertical);

void* loadModel(const std::string& modelName, bool isUnloading, const char* path);
void* loadModel(const std::string& modelName, bool isUnloading, const char* path, std::vector<std::string> animationNames);

void* loadPBRMaterial(const std::string& materialName, bool isUnloading, const std::string& albedoName, const std::string& normalName, const std::string& metallicName, const std::string& roughnessName);
void* loadZellyMaterial(const std::string& materialName, bool isUnloading, glm::vec3 color);

void* loadResource(const std::string& resourceName, bool isUnloading);


namespace Resources
{
	std::map<std::string, void*> resourceMap;

	std::map<std::string, void*>& getResourceMap() { return resourceMap; }

	// ---------------------------------------
	//   The almighty getResource() Function
	// ---------------------------------------
	void* getResource(const std::string& resourceName, const void* compareResource, bool* resourceDiffersAnswer)
	{
		void* resource = findResource(resourceName);
		if (resource != NULL)
		{
			if (resourceDiffersAnswer != nullptr)
				*resourceDiffersAnswer = (resource != compareResource);

			return resource;
		}

		resource = loadResource(resourceName, false);
		registerResource(resourceName, resource);

		if (resourceDiffersAnswer != nullptr)
			*resourceDiffersAnswer = (resource != compareResource);

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
	GLuint toTexture = GL_RGB,
	GLuint minFilter = GL_NEAREST,
	GLuint magFilter = GL_NEAREST,
	GLuint wrapS = GL_REPEAT,
	GLuint wrapT = GL_REPEAT,
	bool flipVertical = true)
{
	if (!isUnloading)
	{
		stbi_set_flip_vertically_on_load(flipVertical);
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

		GLuint fromTexture = GL_RED;
		if (numColorChannels == 2)
			fromTexture = GL_RG;
		else if (numColorChannels == 3)
			fromTexture = GL_RGB;
		else if (numColorChannels == 4)
			fromTexture = GL_RGBA;

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
	GLuint wrapT = GL_REPEAT,
	bool flipVertical = true)
{
	if (!isUnloading)
	{
		stbi_set_flip_vertically_on_load(flipVertical);
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

void* loadModel(const std::string& modelName, bool isUnloading, const char* path, std::vector<std::string> animationNames)
{
	if (!isUnloading)
	{
		Model* model = new Model(path, animationNames);
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
				*(GLuint*)Resources::getResource("shader;pbr"),
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

void* loadZellyMaterial(const std::string& materialName, bool isUnloading, glm::vec3 color)
{
	if (!isUnloading)
	{
		Material* material = new ZellyMaterial(color);
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
	if (resourceName == "shader;pbr")									return loadShaderProgramVF(resourceName, isUnloading, "shaders/pbr.vert", "shaders/pbr.frag");
	if (resourceName == "shader;zelly")									return loadShaderProgramVF(resourceName, isUnloading, "shaders/pbr.vert", "shaders/zelly.frag");
	if (resourceName == "shader;blinnPhong")							return loadShaderProgramVF(resourceName, isUnloading, "shaders/vertex.vert", "shaders/fragment.frag");
	if (resourceName == "shader;blinnPhongSkinned")						return loadShaderProgramVF(resourceName, isUnloading, "shaders/model.vert", "shaders/model.frag");
	if (resourceName == "shader;skybox")								return loadShaderProgramVF(resourceName, isUnloading, "shaders/cubemap.vert", "shaders/skybox.frag");
	if (resourceName == "shader;shadowPass")							return loadShaderProgramVF(resourceName, isUnloading, "shaders/shadow.vert", "shaders/do_nothing.frag");
	if (resourceName == "shader;csmShadowPass")							return loadShaderProgramVGF(resourceName, isUnloading, "shaders/csm_shadow.vert", "shaders/csm_shadow.geom", "shaders/do_nothing.frag");
	if (resourceName == "shader;pointLightShadowPass")					return loadShaderProgramVGF(resourceName, isUnloading, "shaders/point_shadow.vert", "shaders/point_shadow.geom", "shaders/point_shadow.frag");
	if (resourceName == "shader;debugCSM")								return loadShaderProgramVF(resourceName, isUnloading, "shaders/debug_csm.vert", "shaders/debug_csm.frag");
	if (resourceName == "shader;shadowPassSkinned")						return loadShaderProgramVF(resourceName, isUnloading, "shaders/shadow_skinned.vert", "shaders/do_nothing.frag");
	if (resourceName == "shader;text")									return loadShaderProgramVF(resourceName, isUnloading, "shaders/text.vert", "shaders/text.frag");
	//if (resourceName == "shader;hdriGeneration")						return loadShaderProgramVF(resourceName, isUnloading, "shaders/cubemap.vert", "shaders/hdri_equirectangular.frag");
	if (resourceName == "shader;irradianceGeneration")					return loadShaderProgramVF(resourceName, isUnloading, "shaders/cubemap.vert", "shaders/irradiance_convolution.frag");
	if (resourceName == "shader;pbrPrefilterGeneration")				return loadShaderProgramVF(resourceName, isUnloading, "shaders/cubemap.vert", "shaders/prefilter.frag");
	if (resourceName == "shader;brdfGeneration")						return loadShaderProgramVF(resourceName, isUnloading, "shaders/brdf.vert", "shaders/brdf.frag");
	if (resourceName == "shader;bloom_postprocessing")					return loadShaderProgramVF(resourceName, isUnloading, "shaders/bloom_postprocessing.vert", "shaders/bloom_postprocessing.frag");
	if (resourceName == "shader;postprocessing")						return loadShaderProgramVF(resourceName, isUnloading, "shaders/postprocessing.vert", "shaders/postprocessing.frag");
	if (resourceName == "shader;hudUI")									return loadShaderProgramVF(resourceName, isUnloading, "shaders/hudUI.vert", "shaders/hudUI.frag");
#ifdef _DEBUG
	if (resourceName == "shader;selectionSkinnedWireframe")				return loadShaderProgramVF(resourceName, isUnloading, "shaders/pbr.vert", "shaders/color.frag");
#endif

	if (resourceName == "texture;hdrEnvironmentMap")					return loadHDRTexture2D(resourceName, isUnloading, "res/skybox/environment.hdr"/*"res/skybox/rice_field_day_env.hdr"*/, GL_RGB16F, GL_RGB, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

	//
	// Rusty metal pbr
	//
	if (resourceName == "material;pbrRustyMetal")						return loadPBRMaterial(resourceName, isUnloading, "texture;pbrAlbedo", "texture;pbrNormal", "texture;pbrMetalness", "texture;pbrRoughness");

	if (resourceName == "texture;pbrAlbedo")							return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/rustediron2_basecolor.png", GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrNormal")							return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/rustediron2_normal.png", GL_RGB, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrMetalness")							return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/rustediron2_metallic.png", GL_RED, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrRoughness")							return loadTexture2D(resourceName, isUnloading, "res/rusted_iron/rustediron2_roughness.png", GL_RED, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	//
	// Commons
	//
	if (resourceName == "texture;lightIcon")							return loadTexture2D(resourceName, isUnloading, "res/cool_img.png", GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrDefaultNormal")						return loadTexture2D(resourceName, isUnloading, "res/shared_textures/default_normal.png", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbr0Value")							return loadTexture2D(resourceName, isUnloading, "res/shared_textures/0_value.png", GL_RED, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbr0_5Value")							return loadTexture2D(resourceName, isUnloading, "res/shared_textures/0.5_value.png", GL_RED, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	
	//
	// Set pieces
	//
	if (resourceName == "model;waterPuddle")							return loadModel(resourceName, isUnloading, "res/water_pool.glb", { "Idle", "No_Water" });
	if (resourceName == "model;cube")									return loadModel(resourceName, isUnloading, "res/cube.glb");
	if (resourceName == "model;houseInterior")							return loadModel(resourceName, isUnloading, "res/house_w_interior.glb");
	// Custom models vvv
	if (resourceName.rfind("model;custommodel;", 0) == 0)				return loadModel(resourceName, isUnloading, resourceName.substr(18).c_str());

	//
	// Slime Girl Model and Materials
	//
	if (resourceName == "model;slimeGirl")								return loadModel(resourceName, isUnloading, "res/slime_girl/slime_girl.glb", { "Idle", "Running", "Jumping_From_Idle", "Jumping_From_Run", "Land_From_Jumping_From_Idle", "Land_From_Jumping_From_Run", "Land_Hard", "Get_Up_From_Land_Hard", "Draw_Water", "Drink_From_Bottle", "Pick_Up_Bottle"});
	if (resourceName == "model;weaponBottle")							return loadModel(resourceName, isUnloading, "res/slime_girl/weapon_bottle.glb");

	if (resourceName == "material;pbrSlimeBelt")						return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeBeltAlbedo", "texture;pbrSlimeBeltNormal", "texture;pbr0Value", "texture;pbrSlimeBeltRoughness");
	if (resourceName == "texture;pbrSlimeBeltAlbedo")					return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Clay002/1K-JPG/Clay002_1K_Color.jpg", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeBeltNormal")					return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Clay002/1K-JPG/Clay002_1K_NormalGL.jpg", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeBeltRoughness")				return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Clay002/1K-JPG/Clay002_1K_Roughness.jpg", GL_RED, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeBeltAccent")					return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeBeltAccentAlbedo", "texture;pbrSlimeBeltAccentNormal", "texture;pbrSlimeBeltAccentMetalness", "texture;pbrSlimeBeltAccentRoughness");
	if (resourceName == "texture;pbrSlimeBeltAccentAlbedo")				return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Metal007/1K-JPG/Metal007_1K_Color.jpg", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeBeltAccentNormal")				return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Metal007/1K-JPG/Metal007_1K_NormalGL.jpg", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeBeltAccentMetalness")			return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Metal007/1K-JPG/Metal007_1K_Metalness.jpg", GL_RED, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeBeltAccentRoughness")			return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Metal007/1K-JPG/Metal007_1K_Roughness.jpg", GL_RED, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeBody")						return loadZellyMaterial(resourceName, isUnloading, glm::vec3(0, 0, .5));

	if (resourceName == "material;pbrSlimeHair")						return loadZellyMaterial(resourceName, isUnloading, glm::vec3(0.5, 0.5, 1));

	if (resourceName == "material;pbrSlimeEyebrow")						return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeEyebrowAlbedo", "texture;pbrDefaultNormal", "texture;pbr0Value", "texture;pbr0Value");
	if (resourceName == "texture;pbrSlimeEyebrowAlbedo")				return loadTexture2D(resourceName, isUnloading, "res/slime_girl/princess_eyebrow.png", GL_RGBA, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, false);

	if (resourceName == "material;pbrSlimeEye")							return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeEyeAlbedo", "texture;pbrDefaultNormal", "texture;pbr0Value", "texture;pbr0Value");
	if (resourceName == "texture;pbrSlimeEyeAlbedo")					return loadTexture2D(resourceName, isUnloading, "res/slime_girl/eye_blue_solid.png", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, false);

	if (resourceName == "material;pbrSlimeShoeAccent")					return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeShoeAccentAlbedo", "texture;pbrSlimeShoeAccentNormal", "texture;pbr0Value", "texture;pbrSlimeShoeAccentRoughness");
	if (resourceName == "texture;pbrSlimeShoeAccentAlbedo")				return loadTexture2D(resourceName, isUnloading, "res/material_plastic_shoe/albedo.png", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeShoeAccentNormal")				return loadTexture2D(resourceName, isUnloading, "res/material_plastic_shoe/normalGL.png", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeShoeAccentRoughness")			return loadTexture2D(resourceName, isUnloading, "res/material_plastic_shoe/roughness.png", GL_RED, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeShoeBlack")					return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeShoeBlackAlbedo", "texture;pbrSlimeShoeBlackNormal", "texture;pbr0Value", "texture;pbrSlimeShoeBlackRoughness");
	if (resourceName == "texture;pbrSlimeShoeBlackAlbedo")				return loadTexture2D(resourceName, isUnloading, "res/material_plastic_shoe/albedo.png", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeShoeBlackNormal")				return loadTexture2D(resourceName, isUnloading, "res/material_plastic_shoe/normalGL.png", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeShoeBlackRoughness")			return loadTexture2D(resourceName, isUnloading, "res/material_plastic_shoe/roughness.png", GL_RED, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeShoeWhite")					return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeShoeWhiteAlbedo", "texture;pbrSlimeShoeWhiteNormal", "texture;pbr0Value", "texture;pbrSlimeShoeWhiteRoughness");
	if (resourceName == "texture;pbrSlimeShoeWhiteAlbedo")				return loadTexture2D(resourceName, isUnloading, "res/material_plastic_shoe/albedo.png", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeShoeWhiteNormal")				return loadTexture2D(resourceName, isUnloading, "res/material_plastic_shoe/normalGL.png", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeShoeWhiteRoughness")			return loadTexture2D(resourceName, isUnloading, "res/material_plastic_shoe/roughness.png", GL_RED, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeShoeWhite2")					return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeShoeWhite2Albedo", "texture;pbrSlimeShoeWhite2Normal", "texture;pbr0Value", "texture;pbrSlimeShoeWhite2Roughness");
	if (resourceName == "texture;pbrSlimeShoeWhite2Albedo")				return loadTexture2D(resourceName, isUnloading, "res/material_plastic_shoe/albedo.png", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeShoeWhite2Normal")				return loadTexture2D(resourceName, isUnloading, "res/material_plastic_shoe/normalGL.png", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeShoeWhite2Roughness")			return loadTexture2D(resourceName, isUnloading, "res/material_plastic_shoe/roughness.png", GL_RED, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeShorts")						return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeShortsAlbedo", "texture;pbrSlimeShortsNormal", "texture;pbr0Value", "texture;pbrSlimeShortsRoughness");
	if (resourceName == "texture;pbrSlimeShortsAlbedo")					return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Fabric023/1K-JPG/Fabric023_1K_Color.jpg", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeShortsNormal")					return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Fabric023/1K-JPG/Fabric023_1K_NormalGL.jpg", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeShortsRoughness")				return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Fabric023/1K-JPG/Fabric023_1K_Roughness.jpg", GL_RED, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeSweater")						return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeSweaterAlbedo", "texture;pbrSlimeSweaterNormal", "texture;pbr0Value", "texture;pbrSlimeSweaterRoughness");
	if (resourceName == "texture;pbrSlimeSweaterAlbedo")				return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Fabric060/1K-JPG/Fabric060_1K_Color.jpg", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeSweaterNormal")				return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Fabric060/1K-JPG/Fabric060_1K_NormalGL.jpg", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeSweaterRoughness")				return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Fabric060/1K-JPG/Fabric060_1K_Roughness.jpg", GL_RED, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeSweater2")					return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeSweater2Albedo", "texture;pbrSlimeSweater2Normal", "texture;pbr0Value", "texture;pbrSlimeSweater2Roughness");
	if (resourceName == "texture;pbrSlimeSweater2Albedo")				return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Fabric028/1K-JPG/Fabric028_1K_Color.jpg", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeSweater2Normal")				return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Fabric028/1K-JPG/Fabric028_1K_NormalGL.jpg", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeSweater2Roughness")			return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Fabric028/1K-JPG/Fabric028_1K_Roughness.jpg", GL_RED, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeTights")						return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeTightsAlbedo", "texture;pbrDefaultNormal", "texture;pbr0Value", "texture;pbr0_5Value");
	if (resourceName == "texture;pbrSlimeTightsAlbedo")					return loadTexture2D(resourceName, isUnloading, "res/slime_girl/tights_albedo.png", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	//if (resourceName == "texture;pbrSlimeTightsAlbedo")					return loadTexture2D(resourceName, isUnloading, "res/material_plaid/albedo.png", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	//if (resourceName == "texture;pbrSlimeTightsNormal")					return loadTexture2D(resourceName, isUnloading, "res/material_plaid/normalGL.png", GL_RGB, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	//if (resourceName == "texture;pbrSlimeTightsRoughness")				return loadTexture2D(resourceName, isUnloading, "res/material_plaid/roughness.png", GL_RED, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);

	if (resourceName == "material;pbrSlimeVest")						return loadPBRMaterial(resourceName, isUnloading, "texture;pbrSlimeVestAlbedo", "texture;pbrSlimeVestNormal", "texture;pbr0Value", "texture;pbrSlimeVestRoughness");
	if (resourceName == "texture;pbrSlimeVestAlbedo")					return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Fabric018/1K-JPG/Fabric018_1K_Color.jpg", GL_RGB, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeVestNormal")					return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Fabric018/1K-JPG/Fabric018_1K_NormalGL.jpg", GL_RGB, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT);
	if (resourceName == "texture;pbrSlimeVestRoughness")				return loadTexture2D(resourceName, isUnloading, "res/slime_girl/Fabric018/1K-JPG/Fabric018_1K_Roughness.jpg", GL_RED, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT);

	// Out of luck, bud. Try the custom resources yo
	std::cout << "ERROR:: Resource \"" << resourceName << "\" was not found." << std::endl;
	assert(false);
	return nullptr;
}
