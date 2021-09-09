#include "RenderManager.h"

#include "../../MainLoop/MainLoop.h"

#include <string>
#include <cmath>
#include <glm/gtx/scalar_multiplication.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_impl_glfw.h"
#include "../../ImGui/imgui_impl_opengl3.h"
#include "../../ImGui/ImGuizmo.h"

#include "../RenderEngine.resources/ShaderResources.h"
#include "../RenderEngine.resources/TextureResources.h"


#include <assimp/matrix4x4.h>


void renderCube();
void renderQuad();

const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;


RenderManager::RenderManager()
{
	modelPosition = glm::vec3(5.0f, 0.0f, 0.0f);
	pbrModelPosition = glm::vec3(0.0f, 0.5f, 0.0f);
	pbrModelScale = glm::vec3(1.0f);
	planePosition = glm::vec3(0.0f, -3.6f, 0.0f);
	modelEulerAngles = glm::vec3(0.0f, 0.0f, 0.0f);
	lightPosition = glm::vec3(7.0f, 5.0f, 3.0f);

	createProgram();
	createRect();
	createShadowMap();
	createFonts();



	//pbrModel = Model("res/uv_sphere.glb");
}

RenderManager::~RenderManager()
{
	this->finalize();
}




GLuint texture, cubemapTexture;
void RenderManager::createRect()
{
	//
	// Create Quad
	//
	{
		float vertices[] = {
			50.0f,	0.0f,	50.0f,		0.0f, 1.0f, 0.0f,		1.0f, 1.0f,
			-50.0f,	0.0f,	-50.0f,		0.0f, 1.0f, 0.0f,		0.0f, 0.0f,
			-50.0f,	0.0f,	50.0f,		0.0f, 1.0f, 0.0f,		0.0f, 1.0f,
			50.0f,	0.0f,	-50.0f,		0.0f, 1.0f, 0.0f,		1.0f, 0.0f,
			-50.0f,	0.0f,	-50.0f,		0.0f, 1.0f, 0.0f,		0.0f, 0.0f,
			50.0f,	0.0f,	50.0f,		0.0f, 1.0f, 0.0f,		1.0f, 1.0f
		};

		GLuint indices[] = {
			0, 1, 2,
			3, 4, 5
		};

		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	//
	// Create Texture
	//
	texture = TextureResources::getInstance().loadTexture2D("light_icon", "res/cool_img.png", GL_RGBA, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
	//{
	//	int imgWidth, imgHeight, numColorChannels;
	//	
	//	unsigned char* bytes = stbi_load("res/cool_img.png", &imgWidth, &imgHeight, &numColorChannels, STBI_default);

	//	//std::cout << imgWidth << "x" << imgHeight << " (" << numColorChannels << std::endl;

	//	if (bytes == NULL)
	//	{
	//		std::cout << stbi_failure_reason() << std::endl;
	//	}

	//	glGenTextures(1, &texture);
	//	glActiveTexture(GL_TEXTURE0);
	//	glBindTexture(GL_TEXTURE_2D, texture);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);			// TODO: Figure out a way to have imports be SRGB or SRGB_ALPHA and then the output format for this function be set to RGB or RGBA (NOTE: for everywhere that's using this glTexImage2D function.) ALSO NOTE: textures like spec maps and normal maps should not have gamma de-correction applied bc they will not be in sRGB format like normal diffuse textures!
	//	glGenerateMipmap(GL_TEXTURE_2D);

	//	stbi_image_free(bytes);
	//	glBindTexture(GL_TEXTURE_2D, 0);

	//	glUniform1i(
	//		glGetUniformLocation(this->program_id, "tex0"),
	//		0
	//	);
	//}

	//
	// Create skybox from hdr radiance file
	//
	{
		const int renderTextureSize = 512;

		//
		// Load in the hdr file
		//
		hdrTexture = TextureResources::getInstance().loadHDRTexture2D("hdrEnvironmentMap", "res/skybox/environment.hdr", GL_RGB16F, GL_RGB, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		//stbi_set_flip_vertically_on_load(true);
		//int width, height, nrComponents;
		//float* data = stbi_loadf("res/skybox/environment.hdr", &width, &height, &nrComponents, STBI_default);
		////unsigned int hdrTexture;
		//if (data)
		//{
		//	glGenTextures(1, &hdrTexture);
		//	glBindTexture(GL_TEXTURE_2D, hdrTexture);
		//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

		//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//	stbi_image_free(data);
		//}
		//else
		//{
		//	std::cout << "Failed to load HDR image." << std::endl;
		//}

		//
		// Create the framebuffer and renderbuffer to capture the hdr skybox
		//
		unsigned int captureFBO, captureRBO;
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, renderTextureSize, renderTextureSize);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

		//
		// Create cubemap for the framebuffer and renderbuffer
		//
		glGenTextures(1, &envCubemap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		for (unsigned int i = 0; i < 6; ++i)
		{
			// note that we store each face with 16 bit floating point values
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
				renderTextureSize, renderTextureSize, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//
		// Render out the hdr skybox to the framebuffer
		//
		glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		glm::mat4 captureViews[] =
		{
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};
		
		// convert HDR equirectangular environment map to cubemap equivalent
		glUseProgram(hdri_program_id);
		glUniform1i(glGetUniformLocation(this->hdri_program_id, "equirectangularMap"), 0);
		glUniformMatrix4fv(glGetUniformLocation(this->hdri_program_id, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, hdrTexture);

		glViewport(0, 0, renderTextureSize, renderTextureSize); // don't forget to configure the viewport to the capture dimensions.
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		for (unsigned int i = 0; i < 6; i++)
		{
			glUniformMatrix4fv(glGetUniformLocation(this->hdri_program_id, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			renderCube(); // renders a 1x1 cube in NDR
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//
		// Create Irradiance Map
		//
		const int irradianceMapSize = 32;

		glGenTextures(1, &irradianceMap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
		for (unsigned int i = 0; i < 6; i++)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, irradianceMapSize, irradianceMapSize, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, irradianceMapSize, irradianceMapSize);

		glUseProgram(irradiance_program_id);
		glUniform1i(glGetUniformLocation(irradiance_program_id, "environmentMap"), 0);
		glUniformMatrix4fv(glGetUniformLocation(irradiance_program_id, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

		// Render out the irradiance map!
		glViewport(0, 0, irradianceMapSize, irradianceMapSize);
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		for (unsigned int i = 0; i < 6; i++)
		{
			glUniformMatrix4fv(glGetUniformLocation(irradiance_program_id, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			renderCube();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//
		// Create prefilter map for specular roughness
		//
		const int prefilterMapSize = 128;

		//unsigned int prefilterMap;
		glGenTextures(1, &prefilterMap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
		for (unsigned int i = 0; i < 6; i++)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, prefilterMapSize, prefilterMapSize, 0, GL_RGB, GL_FLOAT, nullptr);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);		// Use mips to capture more "diffused" roughness

		//
		// Run Monte-carlo simulation on the environment lighting
		//
		glUseProgram(prefilter_program_id);
		glUniform1i(glGetUniformLocation(prefilter_program_id, "environmentMap"), 0);
		glUniformMatrix4fv(glGetUniformLocation(prefilter_program_id, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		unsigned int maxMipLevels = 5;
		for (unsigned int mip = 0; mip < maxMipLevels; mip++)
		{
			// Resize to mip level size
			unsigned int mipWidth = 128 * std::pow(0.5, mip);
			unsigned int mipHeight = 128 * std::pow(0.5, mip);
			glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
			glViewport(0, 0, mipWidth, mipHeight);

			float roughness = (float)mip / (float)(maxMipLevels - 1);
			glUniform1f(glGetUniformLocation(prefilter_program_id, "roughness"), roughness);
			for (unsigned int i = 0; i < 6; i++)
			{
				glUniformMatrix4fv(glGetUniformLocation(prefilter_program_id, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				renderCube();
			}
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//
		// Create PBR BRDF LUT
		//
		const int brdfLUTSize = 512;

		//unsigned int brdfLUTTexture;
		glGenTextures(1, &brdfLUTTexture);

		glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, brdfLUTSize, brdfLUTSize, 0, GL_RG, GL_FLOAT, 0);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Redo the render buffer to create the brdf texture
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, brdfLUTSize, brdfLUTSize);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

		glViewport(0, 0, brdfLUTSize, brdfLUTSize);
		glUseProgram(brdf_program_id);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		renderQuad();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	//
	// Create PBR Materials
	//
	//{
	//	// Albedo
	//	int imgWidth, imgHeight, numColorChannels;
	//	//stbi_set_flip_vertically_on_load(true);
	//	unsigned char* bytes = stbi_load("res/rusted_iron/rustediron2_basecolor.png", &imgWidth, &imgHeight, &numColorChannels, STBI_default);
	//	//unsigned char* bytes = stbi_load("res/hardwood-floorboards1-bl/hardwood-floorboards1-albedo.png", &imgWidth, &imgHeight, &numColorChannels, STBI_default);

	//	if (bytes == NULL)
	//	{
	//		std::cout << stbi_failure_reason() << std::endl;
	//	}

	//	glGenTextures(1, &pbrAlbedoTexture);
	//	glActiveTexture(GL_TEXTURE0);
	//	glBindTexture(GL_TEXTURE_2D, pbrAlbedoTexture);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);			// TODO: Figure out a way to have imports be SRGB or SRGB_ALPHA and then the output format for this function be set to RGB or RGBA (NOTE: for everywhere that's using this glTexImage2D function.) ALSO NOTE: textures like spec maps and normal maps should not have gamma de-correction applied bc they will not be in sRGB format like normal diffuse textures!
	//	glGenerateMipmap(GL_TEXTURE_2D);

	//	stbi_image_free(bytes);
	//	glBindTexture(GL_TEXTURE_2D, 0);


	//	// Normal
	//	bytes = stbi_load("res/rusted_iron/rustediron2_normal.png", &imgWidth, &imgHeight, &numColorChannels, STBI_default);
	//	//bytes = stbi_load("res/hardwood-floorboards1-bl/hardwood-floorboards1-normal-ogl.png", &imgWidth, &imgHeight, &numColorChannels, STBI_default);

	//	if (bytes == NULL)
	//	{
	//		std::cout << stbi_failure_reason() << std::endl;
	//	}

	//	glGenTextures(1, &pbrNormalTexture);
	//	glActiveTexture(GL_TEXTURE0);
	//	glBindTexture(GL_TEXTURE_2D, pbrNormalTexture);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imgWidth, imgHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, bytes);			// TODO: Figure out a way to have imports be SRGB or SRGB_ALPHA and then the output format for this function be set to RGB or RGBA (NOTE: for everywhere that's using this glTexImage2D function.) ALSO NOTE: textures like spec maps and normal maps should not have gamma de-correction applied bc they will not be in sRGB format like normal diffuse textures!
	//	glGenerateMipmap(GL_TEXTURE_2D);

	//	stbi_image_free(bytes);
	//	glBindTexture(GL_TEXTURE_2D, 0);


	//	// Metalness
	//	bytes = stbi_load("res/rusted_iron/rustediron2_metallic.png", &imgWidth, &imgHeight, &numColorChannels, STBI_default);
	//	//bytes = stbi_load("res/hardwood-floorboards1-bl/hardwood-floorboards1-metallic.png", &imgWidth, &imgHeight, &numColorChannels, STBI_default);

	//	if (bytes == NULL)
	//	{
	//		std::cout << stbi_failure_reason() << std::endl;
	//	}

	//	glGenTextures(1, &pbrMetalnessTexture);
	//	glActiveTexture(GL_TEXTURE0);
	//	glBindTexture(GL_TEXTURE_2D, pbrMetalnessTexture);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, imgWidth, imgHeight, 0, GL_RED, GL_UNSIGNED_BYTE, bytes);			// TODO: Figure out a way to have imports be SRGB or SRGB_ALPHA and then the output format for this function be set to RGB or RGBA (NOTE: for everywhere that's using this glTexImage2D function.) ALSO NOTE: textures like spec maps and normal maps should not have gamma de-correction applied bc they will not be in sRGB format like normal diffuse textures!
	//	glGenerateMipmap(GL_TEXTURE_2D);

	//	stbi_image_free(bytes);
	//	glBindTexture(GL_TEXTURE_2D, 0);


	//	// Roughness
	//	bytes = stbi_load("res/rusted_iron/rustediron2_roughness.png", &imgWidth, &imgHeight, &numColorChannels, STBI_default);
	//	//bytes = stbi_load("res/hardwood-floorboards1-bl/hardwood-floorboards1-roughness.png", &imgWidth, &imgHeight, &numColorChannels, STBI_default);

	//	if (bytes == NULL)
	//	{
	//		std::cout << stbi_failure_reason() << std::endl;
	//	}

	//	glGenTextures(1, &pbrRoughnessTexture);
	//	glActiveTexture(GL_TEXTURE0);
	//	glBindTexture(GL_TEXTURE_2D, pbrRoughnessTexture);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, imgWidth, imgHeight, 0, GL_RED, GL_UNSIGNED_BYTE, bytes);			// TODO: Figure out a way to have imports be SRGB or SRGB_ALPHA and then the output format for this function be set to RGB or RGBA (NOTE: for everywhere that's using this glTexImage2D function.) ALSO NOTE: textures like spec maps and normal maps should not have gamma de-correction applied bc they will not be in sRGB format like normal diffuse textures!
	//	glGenerateMipmap(GL_TEXTURE_2D);

	//	stbi_image_free(bytes);
	//	glBindTexture(GL_TEXTURE_2D, 0);
	//}
}

void RenderManager::createShadowMap()
{
	glGenFramebuffers(1, &depthMapFBO);

	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);		// This prevents this framebuffer from having a "diffuse" layer, since we just need the depth
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderManager::createProgram()
{
	int vShader, fShader;

	this->program_id = glCreateProgram();
	vShader = ShaderResources::compileShader(GL_VERTEX_SHADER, "vertex.vert");
	fShader = ShaderResources::compileShader(GL_FRAGMENT_SHADER, "fragment.frag");
	glAttachShader(this->program_id, vShader);
	glAttachShader(this->program_id, fShader);
	glLinkProgram(this->program_id);
	glDeleteShader(vShader);
	glDeleteShader(fShader);

	this->model_program_id = glCreateProgram();
	vShader = ShaderResources::compileShader(GL_VERTEX_SHADER, "model.vert");
	fShader = ShaderResources::compileShader(GL_FRAGMENT_SHADER, "model.frag");
	glAttachShader(this->model_program_id, vShader);
	glAttachShader(this->model_program_id, fShader);
	glLinkProgram(this->model_program_id);
	glDeleteShader(vShader);
	glDeleteShader(fShader);

	skybox_program_id = glCreateProgram();
	vShader = ShaderResources::compileShader(GL_VERTEX_SHADER, "skybox.vert");
	fShader = ShaderResources::compileShader(GL_FRAGMENT_SHADER, "skybox.frag");
	glAttachShader(skybox_program_id, vShader);
	glAttachShader(skybox_program_id, fShader);
	glLinkProgram(skybox_program_id);
	glDeleteShader(vShader);
	glDeleteShader(fShader);

	shadow_program_id = glCreateProgram();
	vShader = ShaderResources::compileShader(GL_VERTEX_SHADER, "shadow.vert");
	fShader = ShaderResources::compileShader(GL_FRAGMENT_SHADER, "do_nothing.frag");
	glAttachShader(shadow_program_id, vShader);
	glAttachShader(shadow_program_id, fShader);
	glLinkProgram(shadow_program_id);
	glDeleteShader(vShader);
	glDeleteShader(fShader);

	text_program_id = glCreateProgram();
	vShader = ShaderResources::compileShader(GL_VERTEX_SHADER, "text.vert");
	fShader = ShaderResources::compileShader(GL_FRAGMENT_SHADER, "text.frag");
	glAttachShader(text_program_id, vShader);
	glAttachShader(text_program_id, fShader);
	glLinkProgram(text_program_id);
	glDeleteShader(vShader);
	glDeleteShader(fShader);

	hdri_program_id = glCreateProgram();
	vShader = ShaderResources::compileShader(GL_VERTEX_SHADER, "cubemap.vert");
	fShader = ShaderResources::compileShader(GL_FRAGMENT_SHADER, "hdri_equirectangular.frag");
	glAttachShader(hdri_program_id, vShader);
	glAttachShader(hdri_program_id, fShader);
	glLinkProgram(hdri_program_id);
	glDeleteShader(vShader);
	glDeleteShader(fShader);

	irradiance_program_id = glCreateProgram();
	vShader = ShaderResources::compileShader(GL_VERTEX_SHADER, "cubemap.vert");
	fShader = ShaderResources::compileShader(GL_FRAGMENT_SHADER, "irradiance_convolution.frag");
	glAttachShader(irradiance_program_id, vShader);
	glAttachShader(irradiance_program_id, fShader);
	glLinkProgram(irradiance_program_id);
	glDeleteShader(vShader);
	glDeleteShader(fShader);

	prefilter_program_id = glCreateProgram();
	vShader = ShaderResources::compileShader(GL_VERTEX_SHADER, "cubemap.vert");
	fShader = ShaderResources::compileShader(GL_FRAGMENT_SHADER, "prefilter.frag");
	glAttachShader(prefilter_program_id, vShader);
	glAttachShader(prefilter_program_id, fShader);
	glLinkProgram(prefilter_program_id);
	glDeleteShader(vShader);
	glDeleteShader(fShader);

	brdf_program_id = glCreateProgram();
	vShader = ShaderResources::compileShader(GL_VERTEX_SHADER, "brdf.vert");
	fShader = ShaderResources::compileShader(GL_FRAGMENT_SHADER, "brdf.frag");
	glAttachShader(brdf_program_id, vShader);
	glAttachShader(brdf_program_id, fShader);
	glLinkProgram(brdf_program_id);
	glDeleteShader(vShader);
	glDeleteShader(fShader);
}

void RenderManager::createFonts()
{
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
	{
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
		return;
	}

	FT_Face face;
	if (FT_New_Face(ft, "res/arial.ttf", 0, &face))
	{
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
		return;
	}

	FT_Set_Pixel_Sizes(face, 0, 48);

	//
	// Load ASCII first 128 characters (test for now)
	//
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (unsigned char c = 0; c < 128; c++)
	{
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			continue;
		}

		// Generate texture
		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Store character
		TextCharacter newChar = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x
		};
		characters.insert(std::pair<char, TextCharacter>(c, newChar));
	}

	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	//
	// Initialize vao and vbo for drawing text
	//
	glGenVertexArrays(1, &textVAO);
	glGenBuffers(1, &textVBO);
	glBindVertexArray(textVAO);
	glBindBuffer(GL_ARRAY_BUFFER, textVBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}


void RenderManager::render()
{
	renderImGuiPass();

	//
	// Setup projection matrix for rendering
	//
	{
		glm::mat4 lightProjection = glm::ortho(
			-lightOrthoExtent,
			lightOrthoExtent,
			-lightOrthoExtent,
			lightOrthoExtent,
			lightOrthoZNearFar.x,
			lightOrthoZNearFar.y
		);

		glm::mat4 lightView = glm::lookAt(lightPosition,
										glm::vec3(0.0f, 0.0f, 0.0f),
										glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 cameraProjection = MainLoop::getInstance().camera.calculateProjectionMatrix();
		glm::mat4 cameraView = MainLoop::getInstance().camera.calculateViewMatrix();
		updateMatrices(lightProjection, lightView, cameraProjection, cameraView);
	}

	// -----------------------------------------------------------------------------------------------------------------------------
	// Render shadow map to depth framebuffer
	// -----------------------------------------------------------------------------------------------------------------------------
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	renderScene(true);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// -----------------------------------------------------------------------------------------------------------------------------
	// Render scene normally
	// -----------------------------------------------------------------------------------------------------------------------------
	glViewport(0, 0, MainLoop::getInstance().camera.width, MainLoop::getInstance().camera.height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderScene(false);

	// ImGui buffer swap
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());	
}


void RenderManager::updateMatrices(glm::mat4 lightProjection, glm::mat4 lightView, glm::mat4 cameraProjection, glm::mat4 cameraView)
{
	RenderManager::lightProjection = lightProjection;
	RenderManager::lightView = lightView;
	RenderManager::cameraProjection = cameraProjection;
	RenderManager::cameraView = cameraView;
}


void RenderManager::renderScene(bool shadowVersion)
{
	if (!shadowVersion)
	{
		// Draw skybox
		glDepthMask(GL_FALSE);

		glUseProgram(skybox_program_id);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap/*cubemapTexture*/);
		glUniform1i(glGetUniformLocation(this->skybox_program_id, "skyboxTex"), 0);
		glUniformMatrix4fv(glGetUniformLocation(this->skybox_program_id, "skyboxProjViewMatrix"), 1, GL_FALSE, glm::value_ptr(cameraProjection * glm::mat4(glm::mat3(cameraView))));
		renderCube();

		glDepthMask(GL_TRUE);
	}

	//
	// Draw PBR sphere!!!!
	//
	/*int programId = shadowVersion ? this->shadow_program_id : this->pbr_program_id;
	glUseProgram(programId);
	glUniformMatrix4fv(glGetUniformLocation(programId, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightProjection * lightView));
	glUniformMatrix4fv(glGetUniformLocation(programId, "cameraMatrix"), 1, GL_FALSE, glm::value_ptr(cameraProjection * cameraView));

	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pbrAlbedoTexture);
	glUniform1i(glGetUniformLocation(programId, "albedoMap"), 0);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, pbrNormalTexture);
	glUniform1i(glGetUniformLocation(programId, "normalMap"), 1);
	
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, pbrMetalnessTexture);
	glUniform1i(glGetUniformLocation(programId, "metallicMap"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, pbrRoughnessTexture);
	glUniform1i(glGetUniformLocation(programId, "roughnessMap"), 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	glUniform1i(glGetUniformLocation(programId, "irradianceMap"), 4);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	glUniform1i(glGetUniformLocation(programId, "prefilterMap"), 5);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
	glUniform1i(glGetUniformLocation(programId, "brdfLUT"), 6);

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(programId, "shadowMap"), 7);

	glActiveTexture(GL_TEXTURE0);

	

	glm::mat4 modelMatrix =
		glm::translate(pbrModelPosition)
		* glm::scale(glm::mat4(1.0f), pbrModelScale);
	glUniformMatrix4fv(
		glGetUniformLocation(programId, "modelMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(modelMatrix)
	);
	glUniformMatrix3fv(
		glGetUniformLocation(programId, "normalsModelMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(modelMatrix))))
	);
	glUniform3fv(glGetUniformLocation(programId, "lightPositions[0]"), 1, &lightPosition[0]);
	glUniform3f(glGetUniformLocation(programId, "lightColors[0]"), 150.0f, 150.0f, 150.0f);
	glUniform3fv(glGetUniformLocation(programId, "viewPosition"), 1, &camera.position[0]);

	pbrModel.render(programId);*/

	// Draw the model!!!!
	// 
	//programId = shadowVersion ? this->shadow_skinned_program_id : this->model_program_id;
	//glUseProgram(programId);
	/*glUniformMatrix4fv(glGetUniformLocation(programId, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightProjection * lightView));
	glUniformMatrix4fv(glGetUniformLocation(programId, "cameraMatrix"), 1, GL_FALSE, glm::value_ptr(cameraProjection * cameraView));*/

	/*glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(programId, "shadowMap"), 1);
	glActiveTexture(GL_TEXTURE0);*/

	/*modelMatrix =
		glm::translate(modelPosition)
		* glm::eulerAngleXYZ(glm::radians(modelEulerAngles.x), glm::radians(modelEulerAngles.y), glm::radians(modelEulerAngles.z))
		* glm::scale(
			glm::mat4(1.0f),
			glm::vec3(modelScale)
		);
	glUniformMatrix4fv(
		glGetUniformLocation(programId, "modelMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(modelMatrix)
	);
	glUniformMatrix3fv(
		glGetUniformLocation(programId, "normalsModelMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(modelMatrix))))
	);
	glUniform3fv(glGetUniformLocation(programId, "lightPosition"), 1, &lightPosition[0]);
	glUniform3fv(glGetUniformLocation(programId, "viewPosition"), 1, &camera.position[0]);
	glUniform3f(glGetUniformLocation(programId, "lightColor"), 1.0f, 1.0f, 1.0f);

	auto transforms = animator.getFinalBoneMatrices();
	for (int i = 0; i < transforms.size(); i++)
		glUniformMatrix4fv(
			glGetUniformLocation(programId, ("finalBoneMatrices[" + std::to_string(i) + "]").c_str()),
			1,
			GL_FALSE,
			glm::value_ptr(transforms[i])
		);

	tryModel.render(programId);*/

	// Draw quad
	//programId = shadowVersion ? this->shadow_program_id : this->pbr_program_id;
	//glUseProgram(programId);
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, texture);
	//glUniform1i(glGetUniformLocation(programId, "tex0"), 0);
	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	//glUniform1i(glGetUniformLocation(programId, "shadowMap"), 1);
	//glActiveTexture(GL_TEXTURE0);
	//glm::vec3 tint(0.3f, 0.3f, 0.3f);
	//glUniform3fv(glGetUniformLocation(programId, "diffuseTint"), 1, &tint[0]);
	//glUniformMatrix4fv(glGetUniformLocation(programId, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightProjection * lightView));
	//glUniformMatrix4fv(glGetUniformLocation(programId, "cameraMatrix"), 1, GL_FALSE, glm::value_ptr(cameraProjection * cameraView));		// Uniforms must be set after the shader program is set
	//modelMatrix = glm::translate(planePosition);
	//glUniformMatrix4fv(
	//	glGetUniformLocation(programId, "modelMatrix"),
	//	1,
	//	GL_FALSE,
	//	glm::value_ptr(modelMatrix)
	//);
	//glUniformMatrix3fv(
	//	glGetUniformLocation(programId, "normalsModelMatrix"),
	//	1,
	//	GL_FALSE,
	//	glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(modelMatrix))))
	//);
	///*glUniform3fv(glGetUniformLocation(programId, "lightPosition"), 1, &lightPosition[0]);
	//glUniform3fv(glGetUniformLocation(programId, "viewPosition"), 1, &camera.position[0]);
	//glUniform3f(glGetUniformLocation(programId, "lightColor"), 1.0f, 1.0f, 1.0f);*/
	//glBindVertexArray(this->vao);
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


	//
	// Draw Text
	//
	if (!shadowVersion)
	{
		unsigned int programId = text_program_id;
		glm::mat4 modelMatrix =
			glm::translate(textPosition);
		renderText(programId, "Hi there bobby!", modelMatrix, cameraProjection * cameraView, glm::vec3(0.5f, 1.0f, 0.1f));
	}

	for (unsigned int i = 0; i < MainLoop::getInstance().renderObjects.size(); i++)
	{
		MainLoop::getInstance().renderObjects[i]->render(shadowVersion, irradianceMap, prefilterMap, brdfLUTTexture, depthMapTexture);
	}
}


void RenderManager::renderImGuiPass()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::BeginFrame();

	renderImGuiContents();

	// Render
	ImGui::Render();
}


void RenderManager::renderImGuiContents()
{
	static bool showAnalyticsOverlay = true;
	static bool showScenePropterties = true;

	static bool renderWireframeMode = false;

	static bool showShadowMap = false;

	//
	// Menu Bar
	//
	{
		if (ImGui::BeginMainMenuBar())
		{
			//ImGui::begin
			if (ImGui::BeginMenu("File"))
			{
				//ShowExampleMenuFile();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
				if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
				ImGui::Separator();
				if (ImGui::MenuItem("Cut", "CTRL+X")) {}
				if (ImGui::MenuItem("Copy", "CTRL+C")) {}
				if (ImGui::MenuItem("Paste", "CTRL+V")) {}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Window"))
			{
				if (ImGui::MenuItem("Analytics Overlay", NULL, &showAnalyticsOverlay)) {}
				if (ImGui::MenuItem("Scene Properties", NULL, &showScenePropterties)) {}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Rendering"))
			{
				if (ImGui::MenuItem("Wireframe Mode", NULL, &renderWireframeMode))
				{
					glPolygonMode(GL_FRONT_AND_BACK, renderWireframeMode ? GL_LINE : GL_FILL);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
	}

	//
	// Easy Analytics Overlay
	//
	if (showAnalyticsOverlay)
	{
		// FPS counter
		static double prevTime = 0.0, crntTime = 0.0;
		static unsigned int counter = 0;
		static std::string fpsReportString;

		crntTime = glfwGetTime();
		double timeDiff = crntTime - prevTime;
		counter++;
		if (timeDiff >= 1.0 / 7.5)
		{
			std::string fps = std::to_string(1.0 / timeDiff * counter).substr(0, 5);
			std::string ms = std::to_string(timeDiff / counter * 1000).substr(0, 5);
			fpsReportString = "FPS: " + fps + " (" + ms + "ms)";
			prevTime = crntTime;
			counter = 0;
		}


		const float PAD = 10.0f;
		static int corner = 0;
		ImGuiIO& io = ImGui::GetIO();
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
		if (corner != -1)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
			ImVec2 work_size = viewport->WorkSize;
			ImVec2 window_pos, window_pos_pivot;
			window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
			window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
			window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
			window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
			ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
			window_flags |= ImGuiWindowFlags_NoMove;
		}
		ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
		if (ImGui::Begin("Example: Simple overlay", &showAnalyticsOverlay, window_flags))
		{
			ImGui::Text(fpsReportString.c_str());
			ImGui::Separator();
			if (ImGui::IsMousePosValid())
				ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
			else
				ImGui::Text("Mouse Position: <invalid>");
			/*if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Custom", NULL, corner == -1)) corner = -1;
				if (ImGui::MenuItem("Top-left", NULL, corner == 0)) corner = 0;
				if (ImGui::MenuItem("Top-right", NULL, corner == 1)) corner = 1;
				if (ImGui::MenuItem("Bottom-left", NULL, corner == 2)) corner = 2;
				if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
				if (showAnalyticsOverlay && ImGui::MenuItem("Close")) showOverlay = false;
				ImGui::EndPopup();
			}*/
		}
		ImGui::End();
	}

	//
	// Demo window
	//
	static bool show = true;
	if (show)
		ImGui::ShowDemoWindow(&show);

	//
	// Scene Properties window
	//
	if (showScenePropterties)
	{
		if (ImGui::Begin("Scene Properties", &showScenePropterties, ImGuiWindowFlags_AlwaysAutoResize))
		{
			/*static float lightPosVec[3] = { lightPosition.x, lightPosition.y, lightPosition.z };
			ImGui::DragFloat3("Light Position", lightPosVec);

			lightPosition.x = lightPosVec[0];
			lightPosition.y = lightPosVec[1];
			lightPosition.z = lightPosVec[2];

			ImGui::DragFloat3("Model Position", &modelPosition.x);
			ImGui::DragFloat3("Model Euler Angles", &modelEulerAngles.x);

			ImGui::Separator();

			ImGui::DragFloat3("Plane Position", &planePosition.x);

			ImGui::Separator();

			ImGui::DragInt("Bone ID (TEST)", &selectedBone);
			ImGui::InputFloat("Model Scale", &modelScale);
			ImGui::InputFloat("Model Animation Speed", &deltaTimeMultiplier);

			ImGui::Separator();

			ImGui::DragFloat("Light Projection Extent", &lightOrthoExtent);
			ImGui::DragFloat2("Light Projection zNear, zFar", &lightOrthoZNearFar.x);

			ImGui::Checkbox("Display shadow map", &showShadowMap);
			if (showShadowMap)
			{
				ImGui::Image((void*)(intptr_t)depthMapTexture, ImVec2(512, 512));
			}

			ImGui::Separator();*/

			ImGui::DragFloat3("Text Position", &textPosition.x);

			ImGui::Separator();

			/*ImGui::DragFloat3("PBR Position", &pbrModelPosition.x);
			ImGui::DragFloat3("PBR Scale", &pbrModelScale.x);

			ImGui::Separator();*/

			static bool showEnvironmentMap = false;
			ImGui::Checkbox("Display Environ skybox map hdri", &showEnvironmentMap);
			if (showEnvironmentMap)
			{
				ImGui::Image((void*)(intptr_t)hdrTexture, ImVec2(512, 288));
			}

			//
			// Render out the properties panels of everything!!! (in the future have it be a selection process like the unity hierarchy)
			//
			for (unsigned int i = 0; i < MainLoop::getInstance().imguiObjects.size(); i++)
			{
				ImGui::Separator();
				MainLoop::getInstance().imguiObjects[i]->propertyPanelImGui();
			}
		}
		ImGui::End();
	}

	

	//
	// Everything else
	//
	for (unsigned int i = 0; i < MainLoop::getInstance().imguiObjects.size(); i++)
	{
		MainLoop::getInstance().imguiObjects[i]->renderImGui();
	}

	//
	// ImGuizmo
	//
	ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
	ImVec2 work_size = viewport->WorkSize;
	ImGuizmo::SetRect(work_pos.x, work_pos.y, work_size.x, work_size.y);

	/*glm::mat4 identityMatrix(1.0f);																								// I don't really like this grid... it draws over all of the other objects... maybe if I were able to incorporate the depth buffer then I might consider but not really lol
	ImGuizmo::DrawGrid(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection), glm::value_ptr(identityMatrix), 10000.f);*/

	glm::mat4 matrixTransform = MainLoop::getInstance().renderObjects[0]->getTransformAsMatrix();
	ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection), ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::MODE::LOCAL, glm::value_ptr(matrixTransform));

	ImGuizmo::ViewManipulate(glm::value_ptr(cameraView), 8.0f, ImVec2(work_pos.x + work_size.x - 128, work_pos.y), ImVec2(128, 128), 0x10101010);				// NOTE: because the matrix for the cameraview is calculated, there is nothing that this manipulate function does... sad.
}

void RenderManager::renderText(unsigned int programId, std::string text, glm::mat4 modelMatrix, glm::mat4 cameraMatrix, glm::vec3 color)
{
	// Activate corresponding render state
	glUseProgram(programId);
	glUniform3f(glGetUniformLocation(programId, "diffuseTint"), color.x, color.y, color.z);
	glUniformMatrix4fv(
		glGetUniformLocation(programId, "modelMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(modelMatrix)
	);
	glUniformMatrix4fv(glGetUniformLocation(programId, "cameraMatrix"), 1, GL_FALSE, glm::value_ptr(cameraMatrix));
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(textVAO);

	// iterate through all characters
	float x = 0;
	float y = 0;
	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++)
	{
		TextCharacter ch = characters[*c];

		float xpos = x + ch.bearing.x;
		float ypos = y - (ch.size.y - ch.bearing.y);

		float w = ch.size.x;
		float h = ch.size.y;
		// update VBO for each character
		float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },

			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }
		};
		// render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.textureId);
		// update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, textVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		x += (ch.advance >> 6); // bitshift by 6 to get value in pixels (2^6 = 64)
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}



void RenderManager::finalize()
{
	
}


// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}
