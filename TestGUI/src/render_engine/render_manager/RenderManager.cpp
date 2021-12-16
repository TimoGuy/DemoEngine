#include "RenderManager.h"

#include "../../mainloop/MainLoop.h"

#include <string>
#include <cmath>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef _DEBUG
#include "../../imgui/imgui.h"
#include "../../imgui/imgui_impl_glfw.h"
#include "../../imgui/imgui_impl_opengl3.h"
#include "../../imgui/ImGuizmo.h"
#endif

#include "../resources/Resources.h"
#include "../../utils/FileLoading.h"
#include "../../utils/PhysicsUtils.h"
#include "../../utils/GameState.h"

#include <assimp/matrix4x4.h>

// Characters yo
#include "../../objects/PlayerCharacter.h"
#include "../../objects/YosemiteTerrain.h"
#include "../../objects/DirectionalLight.h"
#include "../../objects/PointLight.h"
#include "../../objects/WaterPuddle.h"
#include "../../objects/RiverDropoff.h"


void renderCube();
void renderQuad();

const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
static bool showShadowMapView = false;
bool RenderManager::isWireFrameMode = false;
bool RenderManager::renderPhysicsDebug = false;
bool RenderManager::renderMeshRenderAABB = false;

#ifdef _DEBUG
ImGuizmo::OPERATION transOperation;
#endif


RenderManager::RenderManager()
{
	createShaderPrograms();

	bool firstSkyMap = true;
	for (size_t i = 0; i < numSkyMaps; i++)
	{
		createHDRSkybox(
			firstSkyMap,
			i,
			glm::vec3(
				std::cosf(glm::radians(preBakedSkyMapAngles[i])),
				-std::sinf(glm::radians(preBakedSkyMapAngles[i])),
				0.0f
			)
		);
		firstSkyMap = false;
	}

	createHDRBuffer();
#ifdef _DEBUG
	createPickingBuffer();
#endif
	createFonts();
	createSkeletalAnimationUBO();
}

RenderManager::~RenderManager()
{
	// TODO: add a destructor ya dingus
	destroyHDRBuffer();
}

glm::vec3 sunOrientation(0, 1, 0);
float sunRadius = 0.085f;
glm::vec3 sunColor{ 1, 1, 1 };
glm::vec3 skyColor1{ 0.043 , 0.182 ,0.985 };
glm::vec3 groundColor{ 0.51,0.27, 0.11 };
float sunIntensity = 25;
float globalExposure = 1;
float cloudHeight = -50;
float perlinDim = 2;
float perlinTime = 0;

void RenderManager::createHDRSkybox(bool first, size_t index, const glm::vec3& sunOrientation)
{
	const int renderTextureSize = 512;

	//
	// Create the framebuffer and renderbuffer to capture the hdr skybox
	//
	if (first)
	{
		glGenFramebuffers(1, &hdrPBRGenCaptureFBO);
		glGenRenderbuffers(1, &hdrPBRGenCaptureRBO);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, hdrPBRGenCaptureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, hdrPBRGenCaptureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, renderTextureSize, renderTextureSize);

	if (first)
	{
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, hdrPBRGenCaptureRBO);
	}

	//
	// Create cubemap for the framebuffer and renderbuffer
	//
	if (first)
	{
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
	}

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

	glUseProgram(skybox_program_id);
	glUniform3fv(glGetUniformLocation(this->skybox_program_id, "sunOrientation"), 1, &sunOrientation[0]);
	glUniform1f(glGetUniformLocation(skybox_program_id, "sunRadius"), sunRadius);
	glUniform3fv(glGetUniformLocation(this->skybox_program_id, "sunColor"), 1, &sunColor[0]);
	glUniform3fv(glGetUniformLocation(this->skybox_program_id, "skyColor1"), 1, &skyColor1[0]);
	glUniform3fv(glGetUniformLocation(this->skybox_program_id, "groundColor"), 1, &groundColor[0]);
	glUniform1f(glGetUniformLocation(skybox_program_id, "sunIntensity"), sunIntensity);
	glUniform1f(glGetUniformLocation(skybox_program_id, "globalExposure"), globalExposure);
	glUniform1f(glGetUniformLocation(skybox_program_id, "cloudHeight"), cloudHeight);
	glUniform1f(glGetUniformLocation(skybox_program_id, "perlinDim"), perlinDim);
	glUniform1f(glGetUniformLocation(skybox_program_id, "perlinTime"), perlinTime);
	glUniform1i(glGetUniformLocation(skybox_program_id, "showSun"), 0);				// NOTE: don't include sun in the hdr skies.
	glUniformMatrix4fv(glGetUniformLocation(this->skybox_program_id, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
	glUniform1f(glGetUniformLocation(skybox_program_id, "cameraHeight"), 0.0f);		// NOTE: the hdr skies should get generated with default camera position

	glViewport(0, 0, renderTextureSize, renderTextureSize); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, hdrPBRGenCaptureFBO);
	for (unsigned int i = 0; i < 6; i++)
	{
		glUniformMatrix4fv(glGetUniformLocation(skybox_program_id, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderCube(); // renders a 1x1 cube in NDR
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//
	// Create Irradiance Map
	//
	const int irradianceMapSize = 32;

	glGenTextures(1, &irradianceMap[index]);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap[index]);
	for (unsigned int i = 0; i < 6; i++)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, irradianceMapSize, irradianceMapSize, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, hdrPBRGenCaptureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, hdrPBRGenCaptureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, irradianceMapSize, irradianceMapSize);

	glUseProgram(irradiance_program_id);
	glUniform1i(glGetUniformLocation(irradiance_program_id, "environmentMap"), 0);
	glUniformMatrix4fv(glGetUniformLocation(irradiance_program_id, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	// Render out the irradiance map!
	glViewport(0, 0, irradianceMapSize, irradianceMapSize);
	glBindFramebuffer(GL_FRAMEBUFFER, hdrPBRGenCaptureFBO);
	for (unsigned int i = 0; i < 6; i++)
	{
		glUniformMatrix4fv(glGetUniformLocation(irradiance_program_id, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap[index], 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//
	// Create prefilter map for specular roughness
	//
	const int prefilterMapSize = 128;

	glGenTextures(1, &prefilterMap[index]);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap[index]);
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

	glBindFramebuffer(GL_FRAMEBUFFER, hdrPBRGenCaptureFBO);
	unsigned int maxMipLevels = 5;
	for (unsigned int mip = 0; mip < maxMipLevels; mip++)
	{
		// Resize to mip level size
		unsigned int mipWidth = (unsigned int)(128.0 * std::pow(0.5, mip));
		unsigned int mipHeight = (unsigned int)(128.0 * std::pow(0.5, mip));
		glBindRenderbuffer(GL_RENDERBUFFER, hdrPBRGenCaptureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(maxMipLevels - 1);
		glUniform1f(glGetUniformLocation(prefilter_program_id, "roughness"), roughness);
		for (unsigned int i = 0; i < 6; i++)
		{
			glUniformMatrix4fv(glGetUniformLocation(prefilter_program_id, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap[index], mip);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			renderCube();
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (!first)
		return;

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
	glBindFramebuffer(GL_FRAMEBUFFER, hdrPBRGenCaptureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, hdrPBRGenCaptureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, brdfLUTSize, brdfLUTSize);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

	glViewport(0, 0, brdfLUTSize, brdfLUTSize);
	glUseProgram(brdf_program_id);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderQuad();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderManager::recreateRenderBuffers()
{
	destroyHDRBuffer();
	createHDRBuffer();
#ifdef _DEBUG
	destroyPickingBuffer();
	createPickingBuffer();
#endif
}

#ifdef _DEBUG
void RenderManager::physxVisSetDebugLineList(std::vector<physx::PxDebugLine>* lineList)
{
	delete physxVisDebugLines;			// NOTE: this gets destroyed to prevent any memory leaks
	physxVisDebugLines = lineList;
}
#endif

void RenderManager::createHDRBuffer()
{
	glGenFramebuffers(1, &hdrFBO);
	// Create floating point color buffer
	glGenTextures(1, &hdrColorBuffer);
	glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (GLsizei)MainLoop::getInstance().camera.width, (GLsizei)MainLoop::getInstance().camera.height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Create depth buffer (renderbuffer)
	glGenRenderbuffers(1, &hdrDepthRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, hdrDepthRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, (GLsizei)MainLoop::getInstance().camera.width, (GLsizei)MainLoop::getInstance().camera.height);
	// Attach buffers
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorBuffer, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, hdrDepthRBO);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete! (HDR Render Buffer)" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//
	// Create bloom pingpong buffers
	//
	glm::vec2 bufferDimensions(MainLoop::getInstance().camera.width, MainLoop::getInstance().camera.height);
	glGenFramebuffers(bloomBufferCount, bloomFBOs);
	glGenTextures(bloomBufferCount, bloomColorBuffers);
	for (size_t i = 0; i < bloomBufferCount / 2; i++)
	{
		bufferDimensions /= 2.0f;
		for (size_t j = 0; j < 2; j++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, bloomFBOs[i * 2 + j]);
			glBindTexture(GL_TEXTURE_2D, bloomColorBuffers[i * 2 + j]);
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_RGBA16F, (GLsizei)bufferDimensions.x, (GLsizei)bufferDimensions.y, 0, GL_RGBA, GL_FLOAT, NULL
			);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomColorBuffers[i * 2 + j], 0
			);
		}
	}
}

void RenderManager::destroyHDRBuffer()
{
	glDeleteTextures(1, &hdrColorBuffer);
	glDeleteRenderbuffers(1, &hdrDepthRBO);
	glDeleteFramebuffers(1, &hdrFBO);

	glDeleteTextures(bloomBufferCount, bloomColorBuffers);
	glDeleteFramebuffers(bloomBufferCount, bloomFBOs);
}

void RenderManager::createShaderPrograms()
{
	skybox_program_id = *(GLuint*)Resources::getResource("shader;skybox");
	shadow_program_id = *(GLuint*)Resources::getResource("shader;shadowPass");
	debug_csm_program_id = *(GLuint*)Resources::getResource("shader;debugCSM");
	text_program_id = *(GLuint*)Resources::getResource("shader;text");
	irradiance_program_id = *(GLuint*)Resources::getResource("shader;irradianceGeneration");
	prefilter_program_id = *(GLuint*)Resources::getResource("shader;pbrPrefilterGeneration");
	brdf_program_id = *(GLuint*)Resources::getResource("shader;brdfGeneration");
	bloom_postprocessing_program_id = *(GLuint*)Resources::getResource("shader;bloom_postprocessing");
	postprocessing_program_id = *(GLuint*)Resources::getResource("shader;postprocessing");
	pbrShaderProgramId = *(GLuint*)Resources::getResource("shader;pbr");
	hudUIProgramId = *(GLuint*)Resources::getResource("shader;hudUI");
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
		TextCharacter newChar =
		{
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			(unsigned int)face->glyph->advance.x
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
#ifdef _DEBUG
	renderImGuiPass();

	//
	// @Debug: reload shaders
	//
	createShaderPrograms();
#endif

	//
	// Keyboard shortcuts for wireframe and physics debug
	// 
	// NOTE: the physics debug and mesh AABB debug views don't work in release mode bc imgui is deactivated completely in release mode
	//
	static bool prevF1Keypressed = GLFW_RELEASE;
	bool f1Keypressed = glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_F1);
	if (prevF1Keypressed == GLFW_RELEASE && f1Keypressed == GLFW_PRESS)
	{
		isWireFrameMode = !isWireFrameMode;
	}
	prevF1Keypressed = f1Keypressed;

	static bool prevF2Keypressed = GLFW_RELEASE;
	bool f2Keypressed = glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_F2);
	if (prevF2Keypressed == GLFW_RELEASE && f2Keypressed == GLFW_PRESS)
	{
		renderPhysicsDebug = !renderPhysicsDebug;
	}
	prevF2Keypressed = f2Keypressed;

	static bool prevF3Keypressed = GLFW_RELEASE;
	bool f3Keypressed = glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_F3);
	if (prevF3Keypressed == GLFW_RELEASE && f3Keypressed == GLFW_PRESS)
	{
		renderMeshRenderAABB = !renderMeshRenderAABB;
	}
	prevF3Keypressed = f3Keypressed;

	//
	// Setup projection matrix for rendering
	//
	{
		glm::mat4 cameraProjection = MainLoop::getInstance().camera.calculateProjectionMatrix();
		glm::mat4 cameraView = MainLoop::getInstance().camera.calculateViewMatrix();
		updateMatrices(cameraProjection, cameraView);
	}

	//
	// Render shadow map(s) to depth framebuffer(s)
	//
	for (size_t i = 0; i < MainLoop::getInstance().lightObjects.size(); i++)
	{
		if (!MainLoop::getInstance().lightObjects[i]->castsShadows)
			continue;

		MainLoop::getInstance().lightObjects[i]->renderPassShadowMap();
	}

#ifdef _DEBUG
	//
	// Render Picking texture
	//
	if (DEBUGdoPicking)
	{
		if (!MainLoop::getInstance().playMode)		// NOTE: no reason in particular for making this !playmode only
		{
			// Render out picking data
			glDisable(GL_BLEND);
			glViewport(0, 0, (GLsizei)MainLoop::getInstance().camera.width, (GLsizei)MainLoop::getInstance().camera.height);
			glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			pickingRenderFormatProgramId = *(GLuint*)Resources::getResource("shader;pickingRenderFormat");
			glUseProgram(pickingRenderFormatProgramId);
			glUniformMatrix4fv(glGetUniformLocation(pickingRenderFormatProgramId, "cameraMatrix"), 1, GL_FALSE, glm::value_ptr(cameraProjection * cameraView));
			for (uint32_t i = 0; i < (uint32_t)MainLoop::getInstance().objects.size(); i++)
			{
				if (i == (uint32_t)currentSelectedObjectIndex)
					continue;

				RenderComponent* rc = MainLoop::getInstance().objects[i]->getRenderComponent();
				if (rc == nullptr)
					continue;

				glUniform1ui(glGetUniformLocation(pickingRenderFormatProgramId, "objectID"), i + 1);
				rc->renderShadow(pickingRenderFormatProgramId);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glEnable(GL_BLEND);

			// Read pixel
			double mouseX, mouseY;
			glfwGetCursorPos(MainLoop::getInstance().window, &mouseX, &mouseY);
			PixelInfo pixInfo = readPixelFromPickingBuffer((uint32_t)mouseX, (uint32_t)(MainLoop::getInstance().camera.height - mouseY - 1));
			size_t id = (size_t)pixInfo.objectID;
			currentSelectedObjectIndex = (int)id - 1;
		}

		// Unset flag
		DEBUGdoPicking = false;
	}
	glClearColor(0.0f, 0.1f, 0.2f, 1.0f);

#endif

	//
	// Render scene normally
	//
	glViewport(0, 0, (GLsizei)MainLoop::getInstance().camera.width, (GLsizei)MainLoop::getInstance().camera.height);
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (isWireFrameMode)	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	renderScene();
	if (isWireFrameMode)	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	renderUI();

#ifdef _DEBUG
	//
	// @Debug: Render wireframe of selected object
	//
	static float selectedColorIntensityTime = -1.15f;
	static int previousSelectedObjInd = -1;

	if (previousSelectedObjInd != currentSelectedObjectIndex)
	{
		// Reset
		previousSelectedObjInd = currentSelectedObjectIndex;
		selectedColorIntensityTime = -1.15f;
	}

	if (currentSelectedObjectIndex >= 0 && currentSelectedObjectIndex < MainLoop::getInstance().objects.size())
	{
		RenderComponent* rc = MainLoop::getInstance().objects[currentSelectedObjectIndex]->getRenderComponent();
		if (rc != nullptr)
		{
			// Render that selected object!!!!
			glDepthMask(GL_FALSE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			{
				float evaluatedIntensityValue = (std::sinf(selectedColorIntensityTime) + 1);
				//std::cout << evaluatedIntensityValue << std::endl;		@DEBUG
				GLuint selectionWireframeProgramId =
					*(GLuint*)Resources::getResource("shader;selectionSkinnedWireframe");
				glUseProgram(selectionWireframeProgramId);
				glUniform4f(glGetUniformLocation(selectionWireframeProgramId, "wireframeColor"), 0.973f, 0.29f, 1.0f, std::clamp(evaluatedIntensityValue, 0.0f, 1.0f));
				glUniform1f(glGetUniformLocation(selectionWireframeProgramId, "colorIntensity"), evaluatedIntensityValue);
				glUniformMatrix4fv(glGetUniformLocation(selectionWireframeProgramId, "cameraMatrix"), 1, GL_FALSE, glm::value_ptr(cameraProjection * cameraView));
				rc->renderShadow(selectionWireframeProgramId);
			}
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDepthMask(GL_TRUE);

			selectedColorIntensityTime += MainLoop::getInstance().deltaTime * 4.0f;
		}
	}
#endif

	//
	// Do bloom: breakdown-preprocessing
	//
	bool firstcopy = true;
	float downscaledFactor = 2.0f;
	for (size_t i = 0; i < bloomBufferCount / 2; i++)
	{
		for (size_t j = 0; j < 3; j++)																		// There are three stages in each pass: 1: copy, 2: horiz gauss, 3: vert gauss
		{
			size_t bloomFBOIndex = i * 2 + j % 2;					// Needs to have a sequence of i(0):  0,1,0; i(1): 2,3,2; i(2): 4,5,4; i(3): 6,7,6; i(4): 8,9,8
			size_t colorBufferIndex = i * 2 - 1 + j;				// Needs to have a sequence of i(0): -1,0,1; i(1): 1,2,3; i(2): 3,4,5; i(3): 5,6,7; i(4): 7,8,9
			GLint stageNumber = (GLint)j + 1;						// Needs to have a sequence of i(n):  1,2,3

			glBindFramebuffer(GL_FRAMEBUFFER, bloomFBOs[bloomFBOIndex]);
			glClear(GL_COLOR_BUFFER_BIT);
			glUseProgram(bloom_postprocessing_program_id);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, firstcopy ? hdrColorBuffer : bloomColorBuffers[colorBufferIndex]);
			glUniform1i(glGetUniformLocation(bloom_postprocessing_program_id, "hdrColorBuffer"), 0);
			glUniform1i(glGetUniformLocation(bloom_postprocessing_program_id, "stage"), stageNumber);
			glUniform1i(glGetUniformLocation(bloom_postprocessing_program_id, "firstcopy"), firstcopy);
			glUniform1f(glGetUniformLocation(bloom_postprocessing_program_id, "downscaledFactor"), downscaledFactor);
			renderQuad();

			firstcopy = false;
		}

		downscaledFactor *= 2.0f;
	}

	//
	// Do bloom: additive color buffer reconstruction		NOTE: the final reconstructed bloom buffer is on [1]
	//
	assert(bloomBufferCount >= 4);											// NOTE: for this algorithm to work, there must be at least 2 passes(aka 4 fbo's) so that there will be a copy into the needed texture for use in the tonemapping pass after this.
	bool firstReconstruction = true;
	downscaledFactor /= 2.0f;
	for (int i = (int)(bloomBufferCount / 2 - 2) * 2; i >= 0; i -= 2)		// NOTE: must use signed int so that it goes negative
	{
		downscaledFactor /= 2.0f;

		size_t bloomFBOIndex = i + 1;
		size_t colorBufferIndex = i;
		size_t smallerColorBufferIndex = i + 3 - (firstReconstruction ? 1 : 0);

		glBindFramebuffer(GL_FRAMEBUFFER, bloomFBOs[bloomFBOIndex]);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(bloom_postprocessing_program_id);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, bloomColorBuffers[colorBufferIndex]);
		glUniform1i(glGetUniformLocation(bloom_postprocessing_program_id, "hdrColorBuffer"), 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, bloomColorBuffers[smallerColorBufferIndex]);
		glUniform1i(glGetUniformLocation(bloom_postprocessing_program_id, "smallerReconstructHDRColorBuffer"), 1);
		glUniform1i(glGetUniformLocation(bloom_postprocessing_program_id, "stage"), 4);
		glUniform1f(glGetUniformLocation(bloom_postprocessing_program_id, "downscaledFactor"), downscaledFactor);
		renderQuad();

		firstReconstruction = false;
	}

	//
	// Do tonemapping and post-processing
	// with the fbo and render to a quad
	//
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(postprocessing_program_id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
	glUniform1i(glGetUniformLocation(postprocessing_program_id, "hdrColorBuffer"), 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bloomColorBuffers[1]);			// 1 is the final color buffer of the reconstructed bloom
	glUniform1i(glGetUniformLocation(postprocessing_program_id, "bloomColorBuffer"), 1);
	glUniform1f(glGetUniformLocation(postprocessing_program_id, "bloomIntensity"), bloomIntensity);
	renderQuad();

#ifdef _DEBUG
	// ImGui buffer swap
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
}


void RenderManager::updateMatrices(glm::mat4 cameraProjection, glm::mat4 cameraView)
{
	RenderManager::cameraProjection = cameraProjection;
	RenderManager::cameraView = cameraView;
}


void RenderManager::renderScene()
{
	//
	// Draw skybox
	//
	glDepthMask(GL_FALSE);

	for (size_t i = 0; i < MainLoop::getInstance().lightObjects.size(); i++)
	{
		if (MainLoop::getInstance().lightObjects[i]->lightType == LightType::DIRECTIONAL)
		{
			sunOrientation = MainLoop::getInstance().lightObjects[i]->facingDirection;		// NOTE: this is normalized already!!!
			break;
		}
	}

	glUseProgram(skybox_program_id);
	glUniform3fv(glGetUniformLocation(this->skybox_program_id, "sunOrientation"), 1, &sunOrientation[0]);
	glUniform1f(glGetUniformLocation(skybox_program_id, "sunRadius"), sunRadius);
	glUniform3fv(glGetUniformLocation(this->skybox_program_id, "sunColor"), 1, &sunColor[0]);
	glUniform3fv(glGetUniformLocation(this->skybox_program_id, "skyColor1"), 1, &skyColor1[0]);
	glUniform3fv(glGetUniformLocation(this->skybox_program_id, "groundColor"), 1, &groundColor[0]);
	glUniform1f(glGetUniformLocation(skybox_program_id, "sunIntensity"), sunIntensity);
	glUniform1f(glGetUniformLocation(skybox_program_id, "globalExposure"), globalExposure);
	glUniform1f(glGetUniformLocation(skybox_program_id, "cloudHeight"), cloudHeight);
	glUniform1f(glGetUniformLocation(skybox_program_id, "perlinDim"), perlinDim);
	glUniform1f(glGetUniformLocation(skybox_program_id, "perlinTime"), perlinTime);
	glUniform1i(glGetUniformLocation(skybox_program_id, "showSun"), 1);
	glUniformMatrix4fv(glGetUniformLocation(this->skybox_program_id, "projection"), 1, GL_FALSE, glm::value_ptr(cameraProjection));
	glUniformMatrix4fv(glGetUniformLocation(this->skybox_program_id, "view"), 1, GL_FALSE, glm::value_ptr(cameraView));
	glUniform1f(glGetUniformLocation(skybox_program_id, "cameraHeight"), MainLoop::getInstance().camera.position.y);
	//perlinTime += MainLoop::getInstance().deltaTime;
	renderCube();

	glDepthMask(GL_TRUE);

	//
	// Choose which prefilter and irradiance maps to use
	//
	for (int i = (int)numSkyMaps - 1; i >= 0; i--)
	{
		if (-sunOrientation.y < std::sinf(glm::radians(preBakedSkyMapAngles[i])))
		{
			whichMap = i;

			if (i + 1 >= numSkyMaps)
				mapInterpolationAmt = 0;
			else
			{
				mapInterpolationAmt =
					1 -
					(-sunOrientation.y - std::sinf(glm::radians(preBakedSkyMapAngles[i + 1]))) /
					(std::sinf(glm::radians(preBakedSkyMapAngles[i])) - std::sinf(glm::radians(preBakedSkyMapAngles[i + 1])));
			}

			break;
		}
	}

	//
	// Compute how much the prefilter and irradiance maps need to spin their input vectors
	//
	glm::vec3 flatSunOrientation = sunOrientation;
	flatSunOrientation.y = 0;
	flatSunOrientation = glm::normalize(flatSunOrientation);
	sunSpinAmount = glm::toMat3(glm::quat(flatSunOrientation, glm::vec3(1, 0, 0)));


	////
	//// @Remember: this is how to: Draw Text
	////
	//if (!shadowVersion)
	//{
	//	unsigned int programId = text_program_id;
	//	glm::mat4 modelMatrix =
	//		glm::translate(textPosition);
	//	renderText(programId, "Hi there bobby!", modelMatrix, cameraProjection * cameraView, glm::vec3(0.5f, 1.0f, 0.1f));
	//}

	// Reset materials at the start of every frame
	Material::resetFlag = true;
	this->repopulateAnimationUBO = true;

	//
	// OPAQUE RENDER QUEUE
	// 
	// (ALSO: pull out transparent objects here and defer them while rendering out the opaque ones lol)
	// Draw objects but cull them
	// (NOTE: the culling step doesn't happen in the shadowmaps)
	//
	ViewFrustum cookedViewFrustum = ViewFrustum::createFrustumFromCamera(MainLoop::getInstance().camera);		// @Optimize: this can be optimized via a mat4 that just changes the initial view frustum
	for (unsigned int i = 0; i < MainLoop::getInstance().renderObjects.size(); i++)
	{
		// NOTE: viewfrustum culling is handled at the mesh level with some magic. Peek in if ya wanna. -Timo
		MainLoop::getInstance().renderObjects[i]->render(&cookedViewFrustum);
	}

	//
	// TRANSPARENT RENDER QUEUE
	// 
	// Take the developed transparent render queue and render it out!
	//
	std::sort(
		transparentCommandingIndices.begin(),
		transparentCommandingIndices.end(),
		[this](const size_t& index1, const size_t& index2)
		{
			return transparentDistancesToCamera[index1] > transparentDistancesToCamera[index2];
		}
	);
	for (size_t& index : transparentCommandingIndices)
	{
		updateSkeletalBonesUBO(transparentBoneMatrixMemAddrs[index]);
		transparentMeshesToRender[index]->render(transparentModelMatrices[index], 0, true);
	}
	transparentCommandingIndices.clear();
	transparentMeshesToRender.clear();
	transparentModelMatrices.clear();
	transparentBoneMatrixMemAddrs.clear();
	transparentDistancesToCamera.clear();

#ifdef _DEBUG
	//
	// @DEBUG: show grid for imguizmo
	//
	static bool imguizmoIsUsingPrevious = false;

	if (ImGuizmo::IsUsing() && currentSelectedObjectIndex >= 0 && currentSelectedObjectIndex < MainLoop::getInstance().objects.size())
	{
		static bool showXGrid = false;
		static bool showYGrid = false;
		static bool showZGrid = false;

		//
		// See which grid to show
		// NOTE: you do NOT wanna see the jank I had to create inside of ImGuizmo.h and ImGuizmo.cpp to make this work. NG NG
		//
		if (!imguizmoIsUsingPrevious)
		{
			// Reset
			showXGrid = false;
			showYGrid = false;
			showZGrid = false;

			// Test what movetype
			ImGuizmo::vec_t gizmoHitProportion;
			int type = ImGuizmo::GetMoveType(transOperation, &gizmoHitProportion);
			if (type == ImGuizmo::MOVETYPE::MT_NONE)
			{
			}
			else if (type == ImGuizmo::MOVETYPE::MT_MOVE_X)
			{
				showYGrid = true;
				showZGrid = true;
			}
			else if (type == ImGuizmo::MOVETYPE::MT_MOVE_Y)
			{
				showXGrid = true;
				showZGrid = true;
			}
			else if (type == ImGuizmo::MOVETYPE::MT_MOVE_Z)
			{
				showXGrid = true;
				showYGrid = true;
			}
			else if (type == ImGuizmo::MOVETYPE::MT_MOVE_YZ)
			{
				showXGrid = true;
			}
			else if (type == ImGuizmo::MOVETYPE::MT_MOVE_ZX)
			{
				showYGrid = true;
			}
			else if (type == ImGuizmo::MOVETYPE::MT_MOVE_XY)
			{
				showZGrid = true;
			}
			else if (type == ImGuizmo::MOVETYPE::MT_MOVE_SCREEN)
			{
				showXGrid = true;
				showYGrid = true;
				showZGrid = true;
			}
		}

		//
		// Draw the grids!!!
		//
		const glm::vec3 positionVector = PhysicsUtils::getPosition(MainLoop::getInstance().objects[currentSelectedObjectIndex]->getTransform());

		glm::mat4 position = glm::translate(glm::mat4(1.0f), positionVector);
		glm::mat4 rotation = glm::toMat4(PhysicsUtils::getRotation(MainLoop::getInstance().objects[currentSelectedObjectIndex]->getTransform()));
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), {100, 100, 100});
		constexpr float divisor = 4.0f;
		LvlGridMaterial* gridMaterial = (LvlGridMaterial*)Resources::getResource("material;lvlGridMaterial");

		if (showZGrid)
		{
			gridMaterial->setTilingAndOffset({ 50, 50, positionVector.x / divisor, positionVector.y / divisor });
			gridMaterial->setColor(glm::vec3(0.1, 0.1, 1) * 5);
			gridMaterial->applyTextureUniforms();
			glUniformMatrix4fv(glGetUniformLocation(gridMaterial->getShaderId(), "modelMatrix"), 1, GL_FALSE, glm::value_ptr(position * rotation * scale));
			renderQuad();
		}

		if (showYGrid)
		{
			gridMaterial->setTilingAndOffset({ 50, 50, positionVector.x / divisor, positionVector.z / divisor });
			gridMaterial->setColor(glm::vec3(0.1, 1, 0.1) * 5);
			gridMaterial->applyTextureUniforms();
			glm::mat4 xRotate = glm::toMat4(glm::quat(glm::radians(glm::vec3(90, 0, 0))));
			glUniformMatrix4fv(glGetUniformLocation(gridMaterial->getShaderId(), "modelMatrix"), 1, GL_FALSE, glm::value_ptr(position * rotation * xRotate * scale));
			renderQuad();
		}

		if (showXGrid)
		{
			gridMaterial->setTilingAndOffset({ 50, 50, -positionVector.z / divisor, positionVector.y / divisor });
			gridMaterial->setColor(glm::vec3(1, 0.1, 0.1) * 5);
			gridMaterial->applyTextureUniforms();
			glm::mat4 zRotate = glm::toMat4(glm::quat(glm::radians(glm::vec3(0, 90, 0))));
			glUniformMatrix4fv(glGetUniformLocation(gridMaterial->getShaderId(), "modelMatrix"), 1, GL_FALSE, glm::value_ptr(position * rotation * zRotate * scale));
			renderQuad();
		}
	}

	imguizmoIsUsingPrevious = ImGuizmo::IsUsing();
#endif

	//
	// @DEBUG: Draw the shadowmaps on a quad on top of everything else
	//
	if (showShadowMapView)
	{
		glUseProgram(debug_csm_program_id);
		glUniform1i(glGetUniformLocation(debug_csm_program_id, "layer"), debugCSMLayerNum);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, MainLoop::getInstance().lightObjects[0]->shadowMapTexture);
		glUniform1i(glGetUniformLocation(debug_csm_program_id, "depthMap"), 0);
		renderQuad();
	}
}


const size_t MAX_LIGHTS = 8;			// @Hardcode: hopefully this limit goes away in the future if we wanna do forward+ rendering
void RenderManager::setupSceneLights(GLuint programId)
{
	// @TODO: make this connect up to a UBO in the future!
	glUseProgram(programId);

	//
	// Reset shadow maps
	//
	for (size_t i = 0; i < MAX_LIGHTS; i++)
	{
		glUniform1i(glGetUniformLocation(programId, "csmShadowMap"), 100);
		glUniform1i(glGetUniformLocation(programId, ("spotLightShadowMaps[" + std::to_string(i) + "]").c_str()), 100);
		glUniform1i(glGetUniformLocation(programId, ("pointLightShadowMaps[" + std::to_string(i) + "]").c_str()), 100);
	}

	//
	// Setup lights and shadows
	//
	const size_t numLights = std::min(MAX_LIGHTS, MainLoop::getInstance().lightObjects.size());
	glUniform1i(glGetUniformLocation(programId, "numLights"), (GLint)numLights);
	glUniform3fv(glGetUniformLocation(programId, "viewPosition"), 1, &MainLoop::getInstance().camera.position[0]);

	const GLuint baseOffset = 9;											// @Hardcode: Bc GL_TEXTURE8 is the highest being used rn		@TODO: FIX THIS HARDCODE!!!!!!!
	GLuint shadowMapTextureIndex = 0;
	bool setupCSM = false;					// NOTE: unfortunately, I can't figure out a way to have multiple directional light csm's, so here's to just one!
	for (size_t i = 0; i < numLights; i++)
	{
		//
		// Figures out casting shadows
		//
		glUniform1i(glGetUniformLocation(programId, ("hasShadow[" + std::to_string(i) + "]").c_str()), MainLoop::getInstance().lightObjects[i]->castsShadows);
		if (MainLoop::getInstance().lightObjects[i]->castsShadows)
		{
			glActiveTexture(GL_TEXTURE0 + baseOffset + shadowMapTextureIndex);

			if (!setupCSM && MainLoop::getInstance().lightObjects[i]->lightType == LightType::DIRECTIONAL)
			{
				glBindTexture(GL_TEXTURE_2D_ARRAY, MainLoop::getInstance().lightObjects[i]->shadowMapTexture);
				glUniform1i(glGetUniformLocation(programId, "csmShadowMap"), baseOffset + shadowMapTextureIndex);

				// DirectionalLight: Setup for csm rendering
				glUniform1i(glGetUniformLocation(programId, "cascadeCount"), (GLint)((DirectionalLightLight*)MainLoop::getInstance().lightObjects[i])->shadowCascadeLevels.size());
				for (size_t j = 0; j < ((DirectionalLightLight*)MainLoop::getInstance().lightObjects[i])->shadowCascadeLevels.size(); ++j)
				{
					glUniform1f(glGetUniformLocation(programId, ("cascadePlaneDistances[" + std::to_string(j) + "]").c_str()), ((DirectionalLightLight*)MainLoop::getInstance().lightObjects[i])->shadowCascadeLevels[j]);
				}
				glUniformMatrix4fv(
					glGetUniformLocation(programId, "cameraView"),
					1,
					GL_FALSE,
					glm::value_ptr(MainLoop::getInstance().camera.calculateViewMatrix())
				);
				glUniform1f(glGetUniformLocation(programId, "nearPlane"), MainLoop::getInstance().camera.zNear);
				glUniform1f(glGetUniformLocation(programId, "farPlane"), MainLoop::getInstance().lightObjects[i]->shadowFarPlane);

				// Set flag
				setupCSM = true;
			}
			else if (MainLoop::getInstance().lightObjects[i]->lightType == LightType::SPOT)
			{
				glBindTexture(GL_TEXTURE_2D, MainLoop::getInstance().lightObjects[i]->shadowMapTexture);
				glUniform1i(glGetUniformLocation(programId, ("spotLightShadowMaps[" + std::to_string(i) + "]").c_str()), baseOffset + shadowMapTextureIndex);
			}
			else if (MainLoop::getInstance().lightObjects[i]->lightType == LightType::POINT)
			{
				glBindTexture(GL_TEXTURE_CUBE_MAP, MainLoop::getInstance().lightObjects[i]->shadowMapTexture);
				glUniform1i(glGetUniformLocation(programId, ("pointLightShadowMaps[" + std::to_string(i) + "]").c_str()), baseOffset + shadowMapTextureIndex);
				glUniform1f(glGetUniformLocation(programId, ("pointLightShadowFarPlanes[" + std::to_string(i) + "]").c_str()), ((PointLightLight*)MainLoop::getInstance().lightObjects[i])->farPlane);
			}

			shadowMapTextureIndex++;
		}

		//
		// Setting up lights
		//
		LightComponent* light = MainLoop::getInstance().lightObjects[i];
		glm::vec3 lightDirection = glm::vec3(0.0f);

		if (light->lightType != LightType::POINT)
			lightDirection = glm::normalize(light->facingDirection);																									// NOTE: If there is no direction (magnitude: 0), then that means it's a spot light ... Check this first in the shader

		glm::vec4 lightPosition = glm::vec4(glm::vec3(light->baseObject->getTransform()[3]), light->lightType == LightType::DIRECTIONAL ? 0.0f : 1.0f);					// NOTE: when a directional light, position doesn't matter, so that's indicated with the w of the vec4 to be 0
		glm::vec3 lightColorWithIntensity = light->color * light->colorIntensity;

		glUniform3fv(glGetUniformLocation(pbrShaderProgramId, ("lightDirections[" + std::to_string(i) + "]").c_str()), 1, &lightDirection[0]);
		glUniform4fv(glGetUniformLocation(pbrShaderProgramId, ("lightPositions[" + std::to_string(i) + "]").c_str()), 1, &lightPosition[0]);
		glUniform3fv(glGetUniformLocation(pbrShaderProgramId, ("lightColors[" + std::to_string(i) + "]").c_str()), 1, &lightColorWithIntensity[0]);
	}

	glActiveTexture(GL_TEXTURE0);
}


void RenderManager::updateSkeletalBonesUBO(const std::vector<glm::mat4>* boneTransforms)
{
	if (!repopulateAnimationUBO && boneTransforms == assignedBoneMatricesMemAddr)
		return;

	assignedBoneMatricesMemAddr = boneTransforms;

	glBindBuffer(GL_UNIFORM_BUFFER, skeletalAnimationUBO);
	for (size_t i = 0; i < boneTransforms->size(); i++)
	{
		glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4x4), sizeof(glm::mat4x4), &(*boneTransforms)[i]);
	}
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void RenderManager::INTERNALaddTransparentMeshToDeferRender(Mesh* mesh, const glm::mat4& modelMatrix)
{
	transparentCommandingIndices.push_back(transparentMeshesToRender.size());
	transparentMeshesToRender.push_back(mesh);
	transparentModelMatrices.push_back(modelMatrix);
	transparentBoneMatrixMemAddrs.push_back(assignedBoneMatricesMemAddr);		// NOTE: This is to save the bone matrix so that we can render in the transparent queue with it (maybe that was self explanatory baaaaka)

	float distanceToCamera = (cameraProjection * cameraView * modelMatrix * glm::vec4(0, 0, 0, 1)).z;
	transparentDistancesToCamera.push_back(distanceToCamera);
}

void RenderManager::renderSceneShadowPass(GLuint shaderProgramId)
{
	//
	// Render everything
	//
	for (unsigned int i = 0; i < MainLoop::getInstance().renderObjects.size(); i++)
	{
		MainLoop::getInstance().renderObjects[i]->renderShadow(shaderProgramId);
	}
}

glm::vec3 staminaBarPosition{0, -200, 0};
glm::vec3 staminaBarColor1{ 0.0588, 0.0588, 0.0588 };
glm::vec3 staminaBarColor2{ 0.3216, 0.7765, 0.3647 };
glm::vec3 staminaBarColor3{ 0.1686, 0.4275, 0.1922 };
glm::vec3 staminaBarColor4{ 0.5804, 0.05098, 0.05098 };
float staminaBarDepleteColorIntensity = 1024.0f;		// Looks like a lightsaber

void RenderManager::renderUI()
{
	const float camWidth = MainLoop::getInstance().camera.width;
	const float camHeight = MainLoop::getInstance().camera.height;

	if (camWidth == 0.0f || camHeight == 0.0f)
		return;

	//
	// Render UI Stamina Bar
	//
	const float referenceScreenHeight = 500.0f;
	const glm::vec2 staminaBarExtents{ 250.0f, 3.0f };
	const float padding = 1.0f;
	const float staminaAmountFilled = (float)GameState::getInstance().currentPlayerStaminaAmount / (float)GameState::getInstance().maxPlayerStaminaAmount;
	const float staminaDepleteChaser = (float)GameState::getInstance().playerStaminaDepleteChaser / (float)GameState::getInstance().maxPlayerStaminaAmount;

	const float cameraAspectRatio = camWidth / camHeight;
	glm::mat4 viewMatrix =
		glm::ortho(
			-referenceScreenHeight * cameraAspectRatio / 2.0f,
			referenceScreenHeight * cameraAspectRatio / 2.0f,
			-referenceScreenHeight / 2.0f,
			referenceScreenHeight / 2.0f,
			-1.0f,
			1.0f
		);
	glm::mat4 modelMatrix = glm::scale(glm::translate(glm::mat4(1.0f), staminaBarPosition), glm::vec3(staminaBarExtents.x + padding, staminaBarExtents.y + padding, 1.0f));		// @NOTE: remember that renderQuad();'s quad is (-1, 1) width and height, so it has a width/height of 2, not 1

	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(hudUIProgramId);
	glUniform1f(glGetUniformLocation(hudUIProgramId, "padding"), padding);
	glUniform2fv(glGetUniformLocation(hudUIProgramId, "staminaBarExtents"), 1, &staminaBarExtents[0]);
	glUniform1f(glGetUniformLocation(hudUIProgramId, "staminaAmountFilled"), staminaAmountFilled);
	glUniform1f(glGetUniformLocation(hudUIProgramId, "staminaDepleteChaser"), staminaDepleteChaser);
	glUniform3fv(glGetUniformLocation(hudUIProgramId, "staminaBarColor1"), 1, &staminaBarColor1[0]);
	glUniform3fv(glGetUniformLocation(hudUIProgramId, "staminaBarColor2"), 1, &staminaBarColor2[0]);
	glUniform3fv(glGetUniformLocation(hudUIProgramId, "staminaBarColor3"), 1, &staminaBarColor3[0]);
	glUniform3fv(glGetUniformLocation(hudUIProgramId, "staminaBarColor4"), 1, &staminaBarColor4[0]);
	glUniform1f(glGetUniformLocation(hudUIProgramId, "staminaBarDepleteColorIntensity"), staminaBarDepleteColorIntensity);
	glUniformMatrix4fv(glGetUniformLocation(hudUIProgramId, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(glGetUniformLocation(hudUIProgramId, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
	renderQuad();

	GameState::getInstance().updateStaminaDepletionChaser(MainLoop::getInstance().deltaTime);
}

// TODO: REMEMBER THIS!!!!
//if (ImGui::GetIO().WantCaptureMouse ||
//	ImGuizmo::IsOver() ||
//	ImGuizmo::IsUsing())
//	return;

#ifdef _DEBUG
void RenderManager::renderImGuiPass()
{
	renderImGuiContents();

	// Render Imgui
	ImGui::Render();

	// Update and Render additional Platform Windows for Imgui
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}


void RenderManager::renderImGuiContents()
{
	static bool showAnalyticsOverlay = true;
	static bool showScenePropterties = true;
	static bool showObjectSelectionWindow = true;
	static bool showLoadedResourcesWindow = true;
	static bool showMaterialsManager = true;

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
				if (ImGui::MenuItem("Open...", "CTRL+O"))
				{
					FileLoading::getInstance().loadFileWithPrompt();
				}

				bool enableSave = FileLoading::getInstance().isCurrentPathValid();
				if (ImGui::MenuItem("Save", "CTRL+S", false, enableSave))
				{
					FileLoading::getInstance().saveFile(false);
				}

				if (ImGui::MenuItem("Save As...", "CTRL+SHIFT+S"))
				{
					FileLoading::getInstance().saveFile(true);
				}
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

				ImGui::Separator();
				if (ImGui::MenuItem("Duplicate", "CTRL+D", false, currentSelectedObjectIndex >= 0 && currentSelectedObjectIndex < MainLoop::getInstance().objects.size()))
				{
					// NOTE: copypasta
					nlohmann::json j = MainLoop::getInstance().objects[currentSelectedObjectIndex]->savePropertiesToJson();
					j["baseObject"].erase("guid");
					FileLoading::getInstance().createObjectWithJson(j);

					currentSelectedObjectIndex = (int)MainLoop::getInstance().objects.size() - 1;
				}
				if (ImGui::MenuItem("Delete", "Del", false, currentSelectedObjectIndex >= 0 && currentSelectedObjectIndex < MainLoop::getInstance().objects.size()))
				{
					// NOTE: This is copypasta
					delete MainLoop::getInstance().objects[currentSelectedObjectIndex];
					currentSelectedObjectIndex = -1;
				}

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Windows"))
			{
				if (ImGui::MenuItem("Analytics Overlay", NULL, &showAnalyticsOverlay)) {}
				if (ImGui::MenuItem("Scene Properties", NULL, &showScenePropterties)) {}
				if (ImGui::MenuItem("Object Selection", NULL, &showObjectSelectionWindow)) {}
				if (ImGui::MenuItem("Loaded Resources", NULL, &showLoadedResourcesWindow)) {}

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Rendering"))
			{
				ImGui::MenuItem("Wireframe Mode", "F1", &isWireFrameMode);
				ImGui::MenuItem("Physics Debug During Playmode", "F2", &renderPhysicsDebug);

				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
	}

	//
	// Analytics Overlay
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
	// @TODO: once everything you need is made, just comment this out (for future reference in the future eh)
	//
	static bool show = true;
	if (show)
		ImGui::ShowDemoWindow(&show);

#ifdef _DEBUG
	//
	// @PHYSX_VISUALIZATION
	//
	if (renderPhysicsDebug &&
		MainLoop::getInstance().playMode &&
		physxVisDebugLines != nullptr)
	{
		std::vector<physx::PxDebugLine> debugLinesCopy = *physxVisDebugLines;		// @NOTE: this is to prevent the debugLines object from getting deleted while this render function is running
		for (size_t i = 0; i < debugLinesCopy.size(); i++)
		{
			//
			// Convert to screen space
			//
			const physx::PxDebugLine& debugLine = debugLinesCopy[i];
			physx::PxU32 lineColor = debugLine.color0;		// @Checkup: would there be any situation where color0 and color1 would differ????

			// Change ugly purple color to the collision green!
			if (lineColor == 0xFFFF00FF)
				lineColor = ImColor::HSV(0.39f, 0.88f, 0.92f);

			bool willBeOnScreen = true;
			glm::vec3 pointsOnScreen[] = {
				MainLoop::getInstance().camera.PositionToClipSpace(PhysicsUtils::toGLMVec3(debugLine.pos0)),
				MainLoop::getInstance().camera.PositionToClipSpace(PhysicsUtils::toGLMVec3(debugLine.pos1))
			};
			for (size_t ii = 0; ii < 2; ii++)
			{
				if (pointsOnScreen[ii].z < 0.0f)
				{
					// Short circuit bc it won't be on screen anymore
					willBeOnScreen = false;
					break;
				}

				pointsOnScreen[ii] /= pointsOnScreen[ii].z;
				pointsOnScreen[ii].x = pointsOnScreen[ii].x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
				pointsOnScreen[ii].y = -pointsOnScreen[ii].y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;
			}

			if (!willBeOnScreen)
				continue;

			ImVec2 point1(pointsOnScreen[0].x, pointsOnScreen[0].y);
			ImVec2 point2(pointsOnScreen[1].x, pointsOnScreen[1].y);
			ImGui::GetBackgroundDrawList()->AddLine(point1, point2, lineColor, 1.0f);
		}
	}
#endif

	//
	// Everything else
	//
	for (unsigned int i = 0; i < MainLoop::getInstance().objects.size(); i++)
	{
		MainLoop::getInstance().objects[i]->imguiRender();
	}

#ifdef _DEBUG
	//
	// Render bounds of all renderobjects
	//
	if (renderMeshRenderAABB)
	{
		for (size_t i = 0; i < MainLoop::getInstance().renderObjects.size(); i++)
		{
			MainLoop::getInstance().renderObjects[i]->TEMPrenderImguiModelBounds();
		}
	}
#endif

	//
	// Object Selection Window
	//
	static int imGuizmoTransformOperation = 0;
	static int imGuizmoTransformMode = 0;
	if (showObjectSelectionWindow)
	{
		if (ImGui::Begin("Scene Object List", &showObjectSelectionWindow, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Combo("##Transform operation combo", &imGuizmoTransformOperation, "Translate\0Rotate\0Scale\0Bounds");
			ImGui::Combo("##Transform mode combo", &imGuizmoTransformMode, "Local\0World");

			GLFWwindow* windowRef = MainLoop::getInstance().window;
			if (currentSelectedObjectIndex >= 0 &&
				currentSelectedObjectIndex < MainLoop::getInstance().objects.size() &&
				!ImGuizmo::IsUsing() &&
				glfwGetMouseButton(windowRef, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
			{
				if (glfwGetKey(windowRef, GLFW_KEY_Q))
					currentSelectedObjectIndex = -1;
				if (glfwGetKey(windowRef, GLFW_KEY_W))
					imGuizmoTransformOperation = 0;
				if (glfwGetKey(windowRef, GLFW_KEY_E))
					imGuizmoTransformOperation = 1;
				if (glfwGetKey(windowRef, GLFW_KEY_R))
					imGuizmoTransformOperation = 2;
				if (glfwGetKey(windowRef, GLFW_KEY_T))
					imGuizmoTransformOperation = 3;
				if (glfwGetKey(windowRef, GLFW_KEY_DELETE))
				{
					// NOTE: This is copypasta
					delete MainLoop::getInstance().objects[currentSelectedObjectIndex];
					currentSelectedObjectIndex = -1;
				}

				//
				// Press ctrl+d to copy a selected object
				//
				{
					static bool objectDupeKeyboardShortcutLock = false;
					if ((glfwGetKey(windowRef, GLFW_KEY_LEFT_CONTROL) || glfwGetKey(windowRef, GLFW_KEY_RIGHT_CONTROL)) && glfwGetKey(windowRef, GLFW_KEY_D))
					{
						if (!objectDupeKeyboardShortcutLock)
						{
							objectDupeKeyboardShortcutLock = true;

							// NOTE: copypasta
							nlohmann::json j = MainLoop::getInstance().objects[currentSelectedObjectIndex]->savePropertiesToJson();
							j["baseObject"].erase("guid");
							FileLoading::getInstance().createObjectWithJson(j);

							currentSelectedObjectIndex = (int)MainLoop::getInstance().objects.size() - 1;
						}
					}
					else
						objectDupeKeyboardShortcutLock = false;
				}
			}

			if (ImGui::BeginListBox("##listbox Scene Objects", ImVec2(300, 25 * ImGui::GetTextLineHeightWithSpacing())))
			{
				//
				// Display all of the objects in the scene
				//
				for (int n = 0; n < MainLoop::getInstance().objects.size(); n++)
				{
					const bool isSelected = (currentSelectedObjectIndex == n);
					if (ImGui::Selectable(
						(MainLoop::getInstance().objects[n]->name + "##" + MainLoop::getInstance().objects[n]->guid).c_str(),
						isSelected
					))
						currentSelectedObjectIndex = n;

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndListBox();
			}

			//
			// Popup for creating objects
			//
			if (ImGui::Button("Add Object.."))
				ImGui::OpenPopup("add_object_popup");
			if (ImGui::BeginPopup("add_object_popup"))
			{
				//
				// @Palette: where to add objects to add in imgui
				//
				BaseObject* newObject = nullptr;
				if (ImGui::Selectable("Player Character"))			newObject = new PlayerCharacter();
				if (ImGui::Selectable("Directional Light"))			newObject = new DirectionalLight(true);
				if (ImGui::Selectable("Point Light"))				newObject = new PointLight(true);
				if (ImGui::Selectable("Yosemite Terrain"))			newObject = new YosemiteTerrain();
				if (ImGui::Selectable("Collectable Water Puddle"))	newObject = new WaterPuddle();
				if (ImGui::Selectable("River Dropoff Area"))		newObject = new RiverDropoff();

				if (newObject != nullptr)
					currentSelectedObjectIndex = (int)MainLoop::getInstance().objects.size() - 1;

				ImGui::EndPopup();
			}
		}
		ImGui::End();
	}

	//
	// Scene Properties window
	//
	if (showScenePropterties)
	{
		if (ImGui::Begin("Scene Properties", &showScenePropterties, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Checkbox("Show shadowmap view", &showShadowMapView);

			static bool showBloomProcessingBuffers = false;
			ImGui::Checkbox("Show Bloom preprocessing buffers", &showBloomProcessingBuffers);
			if (showBloomProcessingBuffers)
			{
				static int colBufNum = 0;
				ImGui::InputInt("Color Buffer Index", &colBufNum);
				ImGui::Image((void*)(intptr_t)bloomColorBuffers[colBufNum], ImVec2(512, 288));
			}

			ImGui::DragFloat("Bloom Intensity", &bloomIntensity, 0.05f, 0.0f, 5.0f);

			ImGui::Separator();
			ImGui::DragFloat2("StaminaBarPosition", &staminaBarPosition[0]);
			ImGui::ColorEdit3("StaminaBarColor1", &staminaBarColor1[0]);
			ImGui::ColorEdit3("StaminaBarColor2", &staminaBarColor2[0]);
			ImGui::ColorEdit3("StaminaBarColor3", &staminaBarColor3[0]);
			ImGui::DragFloat("StaminaDepletecolorIntensity", &staminaBarDepleteColorIntensity);

			ImGui::Separator();
			ImGui::Text("Skybox properties");
			ImGui::DragFloat("Sun Radius", &sunRadius, 0.01f);
			ImGui::ColorEdit3("sunColor", &sunColor[0]);
			ImGui::ColorEdit3("skyColor1", &skyColor1[0]);
			ImGui::ColorEdit3("groundColor", &groundColor[0]);
			ImGui::DragFloat("Sun Intensity", &sunIntensity, 0.01f);
			ImGui::DragFloat("Global Exposure", &globalExposure, 0.01f);
			ImGui::DragFloat("Cloud Height", &cloudHeight, 0.01f);
			ImGui::DragFloat("perlinDim", &perlinDim, 0.01f);
			ImGui::DragFloat("perlinTime", &perlinTime, 0.01f);

			ImGui::DragInt("Which ibl map", (int*)&whichMap, 1.0f, 0, 4);
			ImGui::DragFloat("Map Interpolation amount", &mapInterpolationAmt);

			//
			// Render out the properties panels of selected object
			//
			ImGui::Separator();
			if (currentSelectedObjectIndex >= 0 &&
				currentSelectedObjectIndex < MainLoop::getInstance().objects.size())
				MainLoop::getInstance().objects[currentSelectedObjectIndex]->imguiPropertyPanel();
			else
				ImGui::Text("No object is currently selected");


		}
		ImGui::End();
	}

	//
	// Loaded resources window
	//
	static int currentSelectLoadedResource = -1;
	if (showLoadedResourcesWindow)
	{
		if (ImGui::Begin("Loaded Resources", &showLoadedResourcesWindow, ImGuiWindowFlags_AlwaysAutoResize))
		{
			std::map<std::string, void*>::iterator it;
			static std::string selectedKey;
			std::map<std::string, void*>& resMapRef = Resources::getResourceMap();
			if (ImGui::BeginListBox("##listbox Loaded Resources", ImVec2(300, 25 * ImGui::GetTextLineHeightWithSpacing())))
			{
				//
				// Display all of the objects in the scene
				//
				int index = 0;
				for (it = resMapRef.begin(); it != resMapRef.end(); it++, index++)
				{
					const bool isSelected = (currentSelectLoadedResource == index);
					if (ImGui::Selectable(
						(it->first).c_str(),
						isSelected
					))
					{
						selectedKey = it->first;
						currentSelectLoadedResource = index;
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndListBox();
			}

			//
			// Reload prompt
			//
			if (currentSelectLoadedResource != -1)
			{
				ImGui::Separator();
				if (ImGui::Button("Reload Resource"))
				{
					Resources::reloadResource(selectedKey);
				}
			}

		}
		ImGui::End();
	}

	//
	// Materials manager window
	//
	if (showMaterialsManager)
	{
		if (ImGui::Begin("Materials Manager", &showMaterialsManager, ImGuiWindowFlags_AlwaysAutoResize))
		{
			//
			// List all materials
			//
			//std::map<std::string, void*>::iterator it;
			//static std::string selectedKey;
			//std::map<std::string, void*>& resMapRef = Resources::getResourceMap();
			//if (ImGui::BeginListBox("##listbox Current Materials", ImVec2(300, 25 * ImGui::GetTextLineHeightWithSpacing())))
			//{
			//	int index = 0;
			//	for (it = resMapRef.begin(); it != resMapRef.end(); it++, index++)
			//	{
			//		const bool isSelected = (currentSelectLoadedResource == index);
			//		if (ImGui::Selectable(
			//			(it->first).c_str(),
			//			isSelected
			//		))
			//		{
			//			selectedKey = it->first;
			//			currentSelectLoadedResource = index;
			//		}
			//
			//		// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			//		if (isSelected)
			//			ImGui::SetItemDefaultFocus();
			//	}
			//	ImGui::EndListBox();
			//}

			//
			// Find if a material is being selected
			//
			Material* materialSelected = nullptr;
			std::string materialName;
			if (currentSelectLoadedResource != -1)
			{
				auto iterator = Resources::getResourceMap().begin();
				std::advance(iterator, currentSelectLoadedResource);

				// Check if starts with "material;"
				if (iterator->first.find("material;") == 0)
				{
					materialName = iterator->first;
					materialSelected = (Material*)iterator->second;
				}
			}

			//
			// Display material properties!!!
			//
			if (materialSelected == nullptr)
			{
				ImGui::Text("No material is currently selected from Loaded Resources");
			}
			else
			{
				ImGui::Text((materialName + " -- Properties").c_str());
				ImGui::DragFloat2("Tiling", materialSelected->getTilingPtr(), 0.05f);
				ImGui::DragFloat2("Offset", materialSelected->getOffsetPtr(), 0.05f);
			}
		}
		ImGui::End();
	}

	//
	// ImGuizmo (NOTE: keep this at the very end so that imguizmo stuff can be rendered after everything else in the background draw list has been rendered)
	//
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 work_pos = viewport->WorkPos;			// Use work area to avoid menu-bar/task-bar, if any!
	ImVec2 work_size = viewport->WorkSize;

	if (currentSelectedObjectIndex >= 0 &&
		currentSelectedObjectIndex < MainLoop::getInstance().objects.size())
	{
		ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
		ImGuizmo::SetRect(work_pos.x, work_pos.y, work_size.x, work_size.y);

		transOperation = ImGuizmo::OPERATION::TRANSLATE;
		if (imGuizmoTransformOperation == 1)
			transOperation = ImGuizmo::OPERATION::ROTATE;
		if (imGuizmoTransformOperation == 2)
			transOperation = ImGuizmo::OPERATION::SCALE;
		if (imGuizmoTransformOperation == 3)
			transOperation = ImGuizmo::OPERATION::BOUNDS;

		ImGuizmo::MODE transMode = ImGuizmo::MODE::LOCAL;
		if (imGuizmoTransformMode == 1)
			transMode = ImGuizmo::MODE::WORLD;

		// Implement snapping if holding down control button(s)
		float snapAmount = 0.0f;
		if (glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
			glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
		{
			if (transOperation == ImGuizmo::OPERATION::ROTATE)
				snapAmount = 45.0f;
			else
				snapAmount = 0.5f;
		}
		glm::vec3 snapValues(snapAmount);

		if (!MainLoop::getInstance().playMode)
		{
			glm::mat4 transformCopy1 = MainLoop::getInstance().objects[currentSelectedObjectIndex]->getTransform();
			glm::mat4 transformCopy2 = transformCopy1;
			ImGuizmo::Manipulate(
				glm::value_ptr(cameraView),
				glm::value_ptr(cameraProjection),
				transOperation,
				transMode,
				glm::value_ptr(transformCopy1),
				NULL,
				&snapValues.x
			);

			if (transformCopy1 != transformCopy2)
			{
				MainLoop::getInstance().objects[currentSelectedObjectIndex]->setTransform(transformCopy1);
			}
		}
	}

	ImGuizmo::ViewManipulate(glm::value_ptr(cameraView), 8.0f, ImVec2(work_pos.x + work_size.x - 128, work_pos.y), ImVec2(128, 128), 0x10101010);		// NOTE: because the matrix for the cameraview is calculated, there is nothing that this manipulate function does... sad.
}
#endif

void RenderManager::renderText(unsigned int programId, std::string text, glm::mat4 modelMatrix, glm::mat4 cameraMatrix, glm::vec3 color)		// @Cleanup: this needs to go in some kind of text utils... it's super useful however, soooooo.
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

		float w = (float)ch.size.x;
		float h = (float)ch.size.y;
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

#ifdef _DEBUG
void RenderManager::createPickingBuffer()			// @Copypasta with createHDRBuffer()
{
	glGenFramebuffers(1, &pickingFBO);
	// Create floating point color buffer
	glGenTextures(1, &pickingColorBuffer);
	glBindTexture(GL_TEXTURE_2D, pickingColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, (GLsizei)MainLoop::getInstance().camera.width, (GLsizei)MainLoop::getInstance().camera.height, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Create depth buffer (renderbuffer)
	glGenRenderbuffers(1, &pickingRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, pickingRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, (GLsizei)MainLoop::getInstance().camera.width, (GLsizei)MainLoop::getInstance().camera.height);
	// Attach buffers
	glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pickingColorBuffer, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pickingRBO);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete! (picking Framebuffer)" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderManager::destroyPickingBuffer()
{
	glDeleteTextures(1, &pickingColorBuffer);
	glDeleteRenderbuffers(1, &pickingRBO);
	glDeleteFramebuffers(1, &pickingFBO);
}

PixelInfo RenderManager::readPixelFromPickingBuffer(uint32_t x, uint32_t y)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, pickingFBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	PixelInfo pixel;
	glReadPixels(x, y, 1, 1, GL_RED, GL_FLOAT, &pixel);

	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	return pixel;
}
#endif

void RenderManager::createSkeletalAnimationUBO()
{
	glGenBuffers(1, &skeletalAnimationUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, skeletalAnimationUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4x4) * 100, nullptr, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, skeletalAnimationUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
