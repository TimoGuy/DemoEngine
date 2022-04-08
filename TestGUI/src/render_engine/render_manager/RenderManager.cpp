#include "RenderManager.h"

#include "../../mainloop/MainLoop.h"

#include <string>
#include <cmath>
#include <random>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef _DEVELOP
#include "../../imgui/imgui.h"
#include "../../imgui/imgui_stdlib.h"
#include "../../imgui/imgui_impl_glfw.h"
#include "../../imgui/imgui_impl_opengl3.h"
#include "../../imgui/ImGuizmo.h"
#endif

#include "../material/Texture.h"
#include "../material/Shader.h"
#include "../material/shaderext/ShaderExtCSM_shadow.h"
#include "../material/shaderext/ShaderExtPBR_daynight_cycle.h"
#include "../material/shaderext/ShaderExtShadow.h"
#include "../material/shaderext/ShaderExtSSAO.h"
#include "../material/shaderext/ShaderExtZBuffer.h"
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
#include "../../objects/VoxelGroup.h"
#include "../../objects/Spline.h"


void renderCube();
void renderQuad();

const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
static bool showShadowMapView = false;
static bool showCloudNoiseView = false;
bool RenderManager::isWireFrameMode = false;
bool RenderManager::renderPhysicsDebug = false;
bool RenderManager::renderMeshRenderAABB = false;

#ifdef _DEVELOP
ImGuizmo::OPERATION transOperation;
#endif


RenderManager::RenderManager()
{
	createSkeletalAnimationUBO();
	createLightInformationUBO();
	createCameraInfoUBO();
	createShaderPrograms();
	createFonts();
	createHDRBuffer();
	createLumenAdaptationTextures();
	createCloudNoise();
	//loadResources();

#ifdef _DEVELOP
	createPickingBuffer();
#endif

	//
	// Create all the day/night cycle irradiance and prefilter maps
	// @REFACTOR: place this inside the GlobalLight's constructor and something for the destructor
	//
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
}

RenderManager::~RenderManager()
{
	destroySkeletalAnimationUBO();
	destroyLightInformationUBO();
	destroyCameraInfoUBO();
	destroyShaderPrograms();
	destroyFonts();
	destroyHDRBuffer();
	destroyLumenAdaptationTextures();
	destroyCloudNoise();
	//unloadResources();

#ifdef _DEVELOP
	destroyPickingBuffer();
#endif
}

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

	// Get the nightsky texture from disk
	Texture::setLoadSync(true);
	nightSkybox = (Texture*)Resources::getResource("texture;nightSkybox");
	Texture::setLoadSync(false);

	// Convert HDR dynamic skybox to cubemap equivalent
	skybox_program_id->use();
	skybox_program_id->setVec3("sunOrientation", sunOrientation);
	skybox_program_id->setFloat("sunRadius", skyboxParams.sunRadius);
	skybox_program_id->setFloat("sunAlpha", 0.0f);
	skybox_program_id->setVec3("sunColor", skyboxParams.sunColor);
	skybox_program_id->setVec3("skyColor1", skyboxParams.skyColor1);
	skybox_program_id->setVec3("groundColor", skyboxParams.groundColor);
	skybox_program_id->setFloat("sunIntensity", skyboxParams.sunIntensity);
	skybox_program_id->setFloat("globalExposure", skyboxParams.globalExposure);
	skybox_program_id->setFloat("cloudHeight", skyboxParams.cloudHeight);
	skybox_program_id->setFloat("perlinDim", skyboxParams.perlinDim);
	skybox_program_id->setFloat("perlinTime", skyboxParams.perlinTime);
	skybox_program_id->setMat3("nightSkyTransform", skyboxParams.nightSkyTransform);
	skybox_program_id->setMat4("projection", captureProjection);
	skybox_program_id->setSampler("nightSkybox", nightSkybox->getHandle());

	glViewport(0, 0, renderTextureSize, renderTextureSize); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, hdrPBRGenCaptureFBO);
	for (unsigned int i = 0; i < 6; i++)
	{
		skybox_program_id->setMat4("view", captureViews[i]);
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

	irradiance_program_id->use();
	irradiance_program_id->setMat4("projection", captureProjection);
	irradiance_program_id->setSampler("environmentMap", envCubemap);

	// Render out the irradiance map!
	glViewport(0, 0, irradianceMapSize, irradianceMapSize);
	glBindFramebuffer(GL_FRAMEBUFFER, hdrPBRGenCaptureFBO);
	for (unsigned int i = 0; i < 6; i++)
	{
		irradiance_program_id->setMat4("view", captureViews[i]);
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
	prefilter_program_id->use();
	prefilter_program_id->setSampler("environmentMap", envCubemap);
	prefilter_program_id->setMat4("projection", captureProjection);

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
		prefilter_program_id->setFloat("roughness", roughness);
		for (unsigned int i = 0; i < 6; i++)
		{
			prefilter_program_id->setMat4("view", captureViews[i]);
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
	brdf_program_id->use();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderQuad();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	ShaderExtPBR_daynight_cycle::brdfLUT = brdfLUTTexture;
}

void RenderManager::recreateRenderBuffers()
{
	destroyHDRBuffer();
	createHDRBuffer();
#ifdef _DEVELOP
	destroyPickingBuffer();
	createPickingBuffer();
#endif
}

#ifdef _DEVELOP
std::vector<BaseObject*> RenderManager::getSelectedObjects()
{
	std::vector<BaseObject*> objs;

	for (size_t i = 0; i < selectedObjectIndices.size(); i++)
	{
		size_t selectedIndex = selectedObjectIndices[i];
		if (selectedIndex >= MainLoop::getInstance().objects.size())
			continue;

		objs.push_back(MainLoop::getInstance().objects[selectedIndex]);
	}

	return objs;
}

bool RenderManager::isObjectSelected(size_t index)
{
	return (std::find(selectedObjectIndices.begin(), selectedObjectIndices.end(), index) != selectedObjectIndices.end());
}

bool RenderManager::isObjectSelected(const std::string& guid)
{
	// @NOTE: this is very inefficient, but it's only used in the voxelgroup class in the level editor, so yeah. It should be okay eh.
	for (size_t i = 0; i < selectedObjectIndices.size(); i++)
	{
		if (MainLoop::getInstance().objects[selectedObjectIndices[i]]->guid == guid)
			return true;
	}

	return false;
}

void RenderManager::addSelectObject(size_t index)
{
	if (!isObjectSelected(index))
		selectedObjectIndices.push_back(index);
}

void RenderManager::deselectObject(size_t index)
{
	auto it = std::find(selectedObjectIndices.begin(), selectedObjectIndices.end(), index);
	if (it != selectedObjectIndices.end())
	{
		selectedObjectIndices.erase(it);
	}
}

void RenderManager::deselectAllSelectedObject()
{
	selectedObjectIndices.clear();
}

void RenderManager::deleteAllSelectedObjects()
{
	std::vector<BaseObject*> objs = getSelectedObjects();
	for (size_t i = 0; i < objs.size(); i++)
	{
		delete objs[i];
	}
	deselectAllSelectedObject();
}
#endif

#ifdef _DEVELOP
void RenderManager::physxVisSetDebugLineList(std::vector<physx::PxDebugLine>* lineList)
{
	delete physxVisDebugLines;			// NOTE: this gets destroyed to prevent any memory leaks
	physxVisDebugLines = lineList;
}
#endif

void RenderManager::addTextRenderer(TextRenderer* textRenderer)
{
	textRQ.textRenderers.push_back(textRenderer);
}

void RenderManager::removeTextRenderer(TextRenderer* textRenderer)
{
	textRQ.textRenderers.erase(
		std::remove(
			textRQ.textRenderers.begin(),
			textRQ.textRenderers.end(),
			textRenderer
		),
		textRQ.textRenderers.end()
	);
}

void RenderManager::pushMessage(const std::string& text)
{
	notifHoldTimers.push_back(0);
	notifMessages.push_back(text);
}

void RenderManager::createHDRBuffer()
{
	//
	// Create HDR framebuffer
	//
	glCreateFramebuffers(1, &hdrFBO);
	// Create floating point color buffer
	glCreateTextures(GL_TEXTURE_2D, 1, &hdrColorBuffer);
	glTextureParameteri(hdrColorBuffer, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(hdrColorBuffer, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(hdrColorBuffer, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(hdrColorBuffer, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureStorage2D(hdrColorBuffer, 1, GL_RGBA16F, (GLsizei)MainLoop::getInstance().camera.width, (GLsizei)MainLoop::getInstance().camera.height);
	//glTextureSubImage2D(hdrColorBuffer, 0, 0, 0, (GLsizei)MainLoop::getInstance().camera.width, (GLsizei)MainLoop::getInstance().camera.height, GL_RGBA, GL_FLOAT, NULL);
	// Create depth buffer (renderbuffer)
	glGenRenderbuffers(1, &hdrDepthRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, hdrDepthRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, (GLsizei)MainLoop::getInstance().camera.width, (GLsizei)MainLoop::getInstance().camera.height);
	// Attach buffers
	glNamedFramebufferTexture(hdrFBO, GL_COLOR_ATTACHMENT0, hdrColorBuffer, 0);
	glNamedFramebufferRenderbuffer(hdrFBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, hdrDepthRBO);
	if (glCheckNamedFramebufferStatus(hdrFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete! (HDR Render Buffer)" << std::endl;

	//
	// Create bloom pingpong buffers
	//
	glm::vec2 bufferDimensions(MainLoop::getInstance().camera.width, MainLoop::getInstance().camera.height);
	bufferDimensions /= 2.0f;		// NOTE: Bloom should start at 1/4 the size.

	glGenFramebuffers(bloomBufferCount, bloomFBOs);
	glGenTextures(bloomBufferCount, bloomColorBuffers);
	for (size_t i = 0; i < bloomBufferCount / 2; i++)
	{
		bufferDimensions /= 2.0f;
		for (size_t j = 0; j < 2; j++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, bloomFBOs[i * 2 + j]);
			glBindTexture(GL_TEXTURE_2D, bloomColorBuffers[i * 2 + j]);
			glTexImage2D(		// @TODO: We don't need the alpha channel for this yo.
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

	//
	// Create Z-Prepass framebuffer
	//
	zPrePassDepthTexture =
		new Texture2D(
			(GLsizei)MainLoop::getInstance().camera.width,
			(GLsizei)MainLoop::getInstance().camera.height,
			1,
			GL_DEPTH_COMPONENT24,
			GL_DEPTH_COMPONENT,
			GL_FLOAT,
			nullptr,
			GL_NEAREST,
			GL_NEAREST,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE
		);
	//ssNormalTexture =			@DEPRECATE
	//	new Texture2D(
	//		(GLsizei)MainLoop::getInstance().camera.width,
	//		(GLsizei)MainLoop::getInstance().camera.height,
	//		1,
	//		GL_RGB16F,
	//		GL_RGB,
	//		GL_FLOAT,
	//		nullptr,
	//		GL_NEAREST,
	//		GL_NEAREST,
	//		GL_CLAMP_TO_EDGE,
	//		GL_CLAMP_TO_EDGE
	//	);

	glCreateFramebuffers(1, &zPrePassFBO);
	//glNamedFramebufferTexture(zPrePassFBO, GL_COLOR_ATTACHMENT0, ssNormalTexture->getHandle(), 0);		@DEPRECATE
	glNamedFramebufferTexture(zPrePassFBO, GL_DEPTH_ATTACHMENT, zPrePassDepthTexture->getHandle(), 0);
	if (glCheckNamedFramebufferStatus(zPrePassFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete! (Z-Prepass Framebuffer)" << std::endl;

	ShaderExtZBuffer::depthTexture = zPrePassDepthTexture->getHandle();

	//
	// Create SSAO framebuffer
	//
	ssaoRotationTexture = (Texture*)Resources::getResource("texture;ssaoRotation");
	ssaoTexture =
		new Texture2D(
			(GLsizei)MainLoop::getInstance().camera.width,//(GLsizei)ssaoFBOSize,
			(GLsizei)MainLoop::getInstance().camera.height,//(GLsizei)ssaoFBOSize,
			1,
			GL_R8,
			GL_RED,
			GL_UNSIGNED_BYTE,
			nullptr,
			GL_LINEAR,
			GL_LINEAR,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE
		);
	ssaoBlurTexture =
		new Texture2D(
			(GLsizei)MainLoop::getInstance().camera.width,//(GLsizei)ssaoFBOSize,
			(GLsizei)MainLoop::getInstance().camera.height,//(GLsizei)ssaoFBOSize,
			1,
			GL_R8,
			GL_RED,
			GL_UNSIGNED_BYTE,
			nullptr,
			GL_LINEAR,
			GL_LINEAR,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE
		);

	ShaderExtSSAO::ssaoTexture = ssaoTexture->getHandle();
	
	glCreateFramebuffers(1, &ssaoFBO);
	glNamedFramebufferTexture(ssaoFBO, GL_COLOR_ATTACHMENT0, ssaoTexture->getHandle(), 0);
	if (glCheckNamedFramebufferStatus(ssaoFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete! (SSAO Framebuffer)" << std::endl;
	
	glCreateFramebuffers(1, &ssaoBlurFBO);
	glNamedFramebufferTexture(ssaoBlurFBO, GL_COLOR_ATTACHMENT0, ssaoBlurTexture->getHandle(), 0);
	if (glCheckNamedFramebufferStatus(ssaoBlurFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete! (SSAO Blur Framebuffer)" << std::endl;

	//
	// Create Volumetric Lighting framebuffer
	//
	volumetricLightingStrength = 0.01f;		// @NOTE: I hate how subtle it is, but it just needs to be like this lol (According to Tiffoneus Bamboozler)  -Timo 01-20-2022

	constexpr float volumetricTextureScale = 0.125f;
	volumetricTextureWidth = MainLoop::getInstance().camera.width * volumetricTextureScale;
	volumetricTextureHeight = MainLoop::getInstance().camera.height * volumetricTextureScale;

	volumetricTexture =
		new Texture2D(
			(GLsizei)volumetricTextureWidth,
			(GLsizei)volumetricTextureHeight,
			1,
			GL_R32F,
			GL_RED,
			GL_FLOAT,
			nullptr,
			GL_NEAREST,
			GL_LINEAR,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE
		);
	volumetricBlurTexture =
		new Texture2D(
			(GLsizei)volumetricTextureWidth,
			(GLsizei)volumetricTextureHeight,
			1,
			GL_R32F,
			GL_RED,
			GL_FLOAT,
			nullptr,
			GL_NEAREST,
			GL_LINEAR,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE
		);

	glCreateFramebuffers(1, &volumetricFBO);
	glNamedFramebufferTexture(volumetricFBO, GL_COLOR_ATTACHMENT0, volumetricTexture->getHandle(), 0);
	if (glCheckNamedFramebufferStatus(volumetricFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete! (Volumetric Framebuffer)" << std::endl;

	glCreateFramebuffers(1, &volumetricBlurFBO);
	glNamedFramebufferTexture(volumetricBlurFBO, GL_COLOR_ATTACHMENT0, volumetricBlurTexture->getHandle(), 0);
	if (glCheckNamedFramebufferStatus(volumetricBlurFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete! (Volumetric Blur Framebuffer)" << std::endl;

	//
	// Create cloud raymarching buffer
	//
	cloudEffectTexture =
		new Texture2D(
			(GLsizei)MainLoop::getInstance().camera.width,
			(GLsizei)MainLoop::getInstance().camera.height,
			1,
			GL_RGBA32F,
			GL_RGBA,
			GL_FLOAT,
			nullptr,
			GL_NEAREST,
			GL_NEAREST,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE
		);
	cloudEffectBlurTexture =
		new Texture2D(
			(GLsizei)MainLoop::getInstance().camera.width,
			(GLsizei)MainLoop::getInstance().camera.height,
			1,
			GL_RGBA32F,
			GL_RGBA,
			GL_FLOAT,
			nullptr,
			GL_NEAREST,
			GL_NEAREST,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE
		);

	glCreateFramebuffers(1, &cloudEffectFBO);
	glNamedFramebufferTexture(cloudEffectFBO, GL_COLOR_ATTACHMENT0, cloudEffectTexture->getHandle(), 0);
	if (glCheckNamedFramebufferStatus(cloudEffectFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete! (Cloud Effect Screenspace Framebuffer)" << std::endl;

	glCreateFramebuffers(1, &cloudEffectBlurFBO);
	glNamedFramebufferTexture(cloudEffectBlurFBO, GL_COLOR_ATTACHMENT0, cloudEffectBlurTexture->getHandle(), 0);
	if (glCheckNamedFramebufferStatus(cloudEffectBlurFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete! (Cloud Effect Screenspace Framebuffer)" << std::endl;
}

void RenderManager::destroyHDRBuffer()
{
	delete cloudEffectTexture;
	glDeleteFramebuffers(1, &cloudEffectFBO);
	delete cloudEffectBlurTexture;
	glDeleteFramebuffers(1, &cloudEffectBlurFBO);

	delete volumetricBlurTexture;
	glDeleteFramebuffers(1, &volumetricBlurFBO);
	delete volumetricTexture;
	glDeleteFramebuffers(1, &volumetricFBO);

	delete ssaoBlurTexture;
	glDeleteFramebuffers(1, &ssaoBlurFBO);
	delete ssaoTexture;
	glDeleteFramebuffers(1, &ssaoFBO);

	//delete ssNormalTexture;		@DEPRECATE
	delete zPrePassDepthTexture;
	glDeleteFramebuffers(1, &zPrePassFBO);

	glDeleteTextures(1, &hdrColorBuffer);
	glDeleteRenderbuffers(1, &hdrDepthRBO);
	glDeleteFramebuffers(1, &hdrFBO);

	glDeleteTextures(bloomBufferCount, bloomColorBuffers);
	glDeleteFramebuffers(bloomBufferCount, bloomFBOs);
}

constexpr GLsizei luminanceTextureSize = 64;
void RenderManager::createLumenAdaptationTextures()
{
	// NOTE: these are always gonna be a constant 64x64 mipmapped so yeah, no need to recreate
	hdrLumDownsampling =
		new Texture2D(
			luminanceTextureSize,
			luminanceTextureSize,
			glm::floor(glm::log2((float_t)luminanceTextureSize)) + 1,		// log_2(64) == 6, and add 1 (i.e.: 64, 32, 16, 8, 4, 2, 1 :: 7 levels)
			GL_R16F,
			GL_RED,
			GL_FLOAT,
			nullptr,
			GL_NEAREST,
			GL_NEAREST,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE
		);
	glCreateFramebuffers(1, &hdrLumFBO);
	glNamedFramebufferTexture(hdrLumFBO, GL_COLOR_ATTACHMENT0, hdrLumDownsampling->getHandle(), 0);		// First level will get populated, and when we need to read the luminance, we'll generate mipmaps and get the lowest luminance value
	if (glCheckNamedFramebufferStatus(hdrLumFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete! (HDR Luminance Sampling Framebuffer)" << std::endl;

	// Create Sampling buffer
	glGenTextures(1, &hdrLumAdaptation1x1);		// NOTE: this has to be non-DSA version of creating textures. For some reason it doesn't work with glCreateTextures();
	glTextureView(hdrLumAdaptation1x1, GL_TEXTURE_2D, hdrLumDownsampling->getHandle(), GL_R16F, 6, 1, 0, 1);

	// Create prev/processed buffers (will be used for ping pong)
	hdrLumAdaptationPrevious =
		new Texture2D(
			1,
			1,
			1,
			GL_R16F,
			GL_RED,
			GL_FLOAT,
			nullptr,
			GL_NEAREST,
			GL_NEAREST,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE
		);
	hdrLumAdaptationProcessed =
		new Texture2D(
			1,
			1,
			1,
			GL_R16F,
			GL_RED,
			GL_FLOAT,
			nullptr,
			GL_NEAREST,
			GL_NEAREST,
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE
		);
	const glm::vec4 startingPixel(glm::vec3(0.00005f), 1.0f);		// NOTE: I presume this'll be mega bright
	glTextureSubImage2D(hdrLumAdaptationPrevious->getHandle(), 0, 0, 0, 1, 1, GL_RED, GL_FLOAT, glm::value_ptr(startingPixel));
}

void RenderManager::destroyLumenAdaptationTextures()
{
	glDeleteTextures(1, &hdrLumAdaptation1x1);
	delete hdrLumDownsampling;
	glDeleteFramebuffers(1, &hdrLumFBO);
	delete hdrLumAdaptationPrevious;
	delete hdrLumAdaptationProcessed;
}

// @NOTE: https://www.guerrilla-games.com/media/News/Files/The-Real-time-Volumetric-Cloudscapes-of-Horizon-Zero-Dawn.pdf
constexpr GLsizei cloudNoiseTex1Size = 128;
constexpr GLsizei cloudNoiseTex2Size = 32;
Texture* cloudNoise1Channels[4];
Texture* cloudNoise2Channels[3];
void RenderManager::createCloudNoise()
{
	//
	// @Cloud noise 1:
	//		R: perlin-worley
	//		G: worley	(Same freq. as R)
	//		B: worley	(Medium frequency)
	//		A: worley	(Highest frequency)
	//
	cloudNoise1 =
		new Texture3D(
			cloudNoiseTex1Size,
			cloudNoiseTex1Size,
			cloudNoiseTex1Size,
			1,
			GL_RGBA8,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			nullptr,
			GL_LINEAR,
			GL_LINEAR,
			GL_REPEAT,
			GL_REPEAT,
			GL_REPEAT
		);

	// Create just a temporary UBO to store the worley noise points
	GLuint cloudNoiseUBO;
	glCreateBuffers(1, &cloudNoiseUBO);
	glNamedBufferData(cloudNoiseUBO, sizeof(glm::vec4) * 1024, nullptr, GL_STATIC_DRAW);	 // @NOTE: the int:numPoints variable gets rounded up to 16 bytes in the buffer btw. No packing.
	glBindBufferBase(GL_UNIFORM_BUFFER, 4, cloudNoiseUBO);

	Texture* cloudNoise1FractalOctaves[] = {
		new Texture3D(cloudNoiseTex1Size, cloudNoiseTex1Size, cloudNoiseTex1Size, 1, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, GL_REPEAT),
		new Texture3D(cloudNoiseTex1Size, cloudNoiseTex1Size, cloudNoiseTex1Size, 1, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, GL_REPEAT),
		new Texture3D(cloudNoiseTex1Size, cloudNoiseTex1Size, cloudNoiseTex1Size, 1, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, GL_REPEAT),
	};

	//Texture* cloudNoise1Channels[] = {
	cloudNoise1Channels[0] = new Texture3D(cloudNoiseTex1Size, cloudNoiseTex1Size, cloudNoiseTex1Size, 1, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, GL_REPEAT);   //,
	cloudNoise1Channels[1] = new Texture3D(cloudNoiseTex1Size, cloudNoiseTex1Size, cloudNoiseTex1Size, 1, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, GL_REPEAT);   //,
	cloudNoise1Channels[2] = new Texture3D(cloudNoiseTex1Size, cloudNoiseTex1Size, cloudNoiseTex1Size, 1, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, GL_REPEAT);   //,
	cloudNoise1Channels[3] = new Texture3D(cloudNoiseTex1Size, cloudNoiseTex1Size, cloudNoiseTex1Size, 1, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, GL_REPEAT);   //,
//};

	const size_t channelGridSizes[] = {
		3, 7, 11,
		9, 20, 33,
		12, 24, 40,
		20, 35, 49
	};		// @NOTE: amount^3 must not exceed 1024... bc I can't figure out how to get more than 1024 in a ubo even though the spec says I should have at least 64kb of vram????
	std::vector<glm::vec3> worleyPoints[] = {
		std::vector<glm::vec3>(), std::vector<glm::vec3>(), std::vector<glm::vec3>(),
		std::vector<glm::vec3>(), std::vector<glm::vec3>(), std::vector<glm::vec3>(),
		std::vector<glm::vec3>(), std::vector<glm::vec3>(), std::vector<glm::vec3>(),
		std::vector<glm::vec3>(), std::vector<glm::vec3>(), std::vector<glm::vec3>()
	};

	std::random_device randomDevice;
	std::mt19937 randomEngine(randomDevice());
	std::uniform_real_distribution<> distribution(0.0, 1.0);

	for (size_t i = 0; i < 12; i++)
	{
		size_t worleyPointCount = channelGridSizes[i] * channelGridSizes[i] * channelGridSizes[i];
		for (size_t j = 0; j < glm::min((size_t)1024, worleyPointCount); j++)
			worleyPoints[i].push_back(
				glm::vec3(distribution(randomEngine), distribution(randomEngine), distribution(randomEngine))
			);
	}

	// Render out all the layers with the points!
	// @TODO: I think what needs to happen is to generate a texture that contains all the points for a worley map and then use that texture to generate the actual worley map. This look up image can just be a GL_NEAREST filtered 8192x8192 texture I think. That should be big enough, and then delete it after it's not needed anymore.  -Timo
	//		@RESPONSE: So I think that the current sitation where only 1024 points are inserted and then repeated works just fine actually. This allows for a smaller set of information needed to look thru.  -Timo
	// @TODO: Also I am noting that there is noise stacking on page 33 of the pdf (https://www.guerrilla-games.com/media/News/Files/The-Real-time-Volumetric-Cloudscapes-of-Horizon-Zero-Dawn.pdf) FOR SURE!!!!!
	GLuint noiseFBO;
	glCreateFramebuffers(1, &noiseFBO);
	glViewport(0, 0, cloudNoiseTex1Size, cloudNoiseTex1Size);

	// Render noise textures onto 4 different 8 bit textures
	for (size_t j = 0; j < 4; j++)
	{
		cloudNoiseGenerateShader->use();
		cloudNoiseGenerateShader->setInt("includePerlin", (int)(j == 0));

		// Render each octave (3 total) for one 8 bit noise texture
		size_t numNoiseOctaves = 3;
		for (size_t k = 0; k < numNoiseOctaves; k++)
		{
			cloudNoiseGenerateShader->setInt("gridSize", (int)channelGridSizes[j * numNoiseOctaves + k]);

			assert(worleyPoints[j * numNoiseOctaves + k].size() <= 1024);
			cloudNoiseInfo.worleyPoints.clear();
			for (size_t worleypointcopy = 0; worleypointcopy < worleyPoints[j * numNoiseOctaves + k].size(); worleypointcopy++)
				cloudNoiseInfo.worleyPoints.push_back(glm::vec4(worleyPoints[j * numNoiseOctaves + k][worleypointcopy], 0.0f));
			glNamedBufferSubData(cloudNoiseUBO, 0, sizeof(glm::vec4) * cloudNoiseInfo.worleyPoints.size(), glm::value_ptr(cloudNoiseInfo.worleyPoints[0]));	 // @NOTE: the offset is sizeof(glm::vec4) bc the memory layout std140 rounds up the 4 bytes of the sizeof(int) up to 16 bytes (size of a vec4)

			//
			// Render each layer with the noise profile
			//
			for (size_t i = 0; i < cloudNoiseTex1Size; i++)
			{
				cloudNoiseGenerateShader->setFloat("currentRenderDepth", (float)i / (float)cloudNoiseTex1Size);

				glNamedFramebufferTextureLayer(noiseFBO, GL_COLOR_ATTACHMENT0, cloudNoise1FractalOctaves[k]->getHandle(), 0, i);
				auto status = glCheckNamedFramebufferStatus(noiseFBO, GL_FRAMEBUFFER);
				if (status != GL_FRAMEBUFFER_COMPLETE)
					std::cout << "Framebuffer not complete! (Worley Noise FBO) (Layer: " << i << ") (Channel: " << j << ") (Octave: " << k << ")" << std::endl;

				glBindFramebuffer(GL_FRAMEBUFFER, noiseFBO);
				glClear(GL_COLOR_BUFFER_BIT);
				renderQuad();
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		}

		//
		// Combine octaves into a single noise channel
		//
		for (size_t i = 0; i < cloudNoiseTex1Size; i++)
		{
			cloudNoiseFractalShader->use();
			cloudNoiseFractalShader->setFloat("currentRenderDepth", (float)i / (float)cloudNoiseTex1Size);
			cloudNoiseFractalShader->setSampler("sample1", cloudNoise1FractalOctaves[0]->getHandle());
			cloudNoiseFractalShader->setSampler("sample2", cloudNoise1FractalOctaves[1]->getHandle());
			cloudNoiseFractalShader->setSampler("sample3", cloudNoise1FractalOctaves[2]->getHandle());
			glNamedFramebufferTextureLayer(noiseFBO, GL_COLOR_ATTACHMENT0, cloudNoise1Channels[j]->getHandle(), 0, i);
			auto status = glCheckNamedFramebufferStatus(noiseFBO, GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "Framebuffer not complete! (Worley Noise FBO) (Layer: " << i << ") (Channel: " << j << ")" << std::endl;

			glBindFramebuffer(GL_FRAMEBUFFER, noiseFBO);
			glClear(GL_COLOR_BUFFER_BIT);
			renderQuad();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	//
	// Render all 4 of the noise textures onto a single RGBA texture
	//
	for (size_t i = 0; i < cloudNoiseTex1Size; i++)
	{
		cloudNoiseCombineShader->use();
		cloudNoiseCombineShader->setFloat("currentRenderDepth", (float)i / (float)cloudNoiseTex1Size);
		cloudNoiseCombineShader->setSampler("R", cloudNoise1Channels[0]->getHandle());
		cloudNoiseCombineShader->setSampler("G", cloudNoise1Channels[1]->getHandle());
		cloudNoiseCombineShader->setSampler("B", cloudNoise1Channels[2]->getHandle());
		cloudNoiseCombineShader->setSampler("A", cloudNoise1Channels[3]->getHandle());

		glNamedFramebufferTextureLayer(noiseFBO, GL_COLOR_ATTACHMENT0, cloudNoise1->getHandle(), 0, i);
		auto status = glCheckNamedFramebufferStatus(noiseFBO, GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete! (Worley Noise FBO) (Layer: " << i << ") (Channel: COMBINER)" << std::endl;

		glBindFramebuffer(GL_FRAMEBUFFER, noiseFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		renderQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	//
	// Cloud noise 2:
	//		R: worley	(Lowest frequency)
	//		G: worley
	//		B: worley	(Highest frequency)
	//
	cloudNoise2 =
		new Texture3D(
			cloudNoiseTex2Size,
			cloudNoiseTex2Size,
			cloudNoiseTex2Size,
			1,
			GL_RGB8,
			GL_RGB,
			GL_UNSIGNED_BYTE,
			nullptr,
			GL_LINEAR,
			GL_LINEAR,
			GL_REPEAT,
			GL_REPEAT,
			GL_REPEAT
		);

	Texture* cloudNoise2FractalOctaves[] = {
		new Texture3D(cloudNoiseTex2Size, cloudNoiseTex2Size, cloudNoiseTex2Size, 1, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, GL_REPEAT),
		new Texture3D(cloudNoiseTex2Size, cloudNoiseTex2Size, cloudNoiseTex2Size, 1, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, GL_REPEAT),
		new Texture3D(cloudNoiseTex2Size, cloudNoiseTex2Size, cloudNoiseTex2Size, 1, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, GL_REPEAT),
	};

	//Texture* cloudNoise2Channels[] = {
		cloudNoise2Channels[0] = new Texture3D(cloudNoiseTex2Size, cloudNoiseTex2Size, cloudNoiseTex2Size, 1, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, GL_REPEAT);   //,
		cloudNoise2Channels[1] = new Texture3D(cloudNoiseTex2Size, cloudNoiseTex2Size, cloudNoiseTex2Size, 1, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, GL_REPEAT);   //,
		cloudNoise2Channels[2] = new Texture3D(cloudNoiseTex2Size, cloudNoiseTex2Size, cloudNoiseTex2Size, 1, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr, GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, GL_REPEAT);   //,
	//};

	// @COPYPASTA: Reinsert/recalculate the randomized points for worley noise
	for (size_t i = 0; i < 9; i++)
	{
		size_t worleyPointCount = channelGridSizes[i] * channelGridSizes[i] * channelGridSizes[i];
		worleyPoints[i].clear();
		for (size_t j = 0; j < glm::min((size_t)1024, worleyPointCount); j++)
			worleyPoints[i].push_back(
				glm::vec3(distribution(randomEngine), distribution(randomEngine), distribution(randomEngine))
			);
	}

	// Render it out!
	glViewport(0, 0, cloudNoiseTex2Size, cloudNoiseTex2Size);

	// Render noise textures onto 3 different 8 bit textures
	for (size_t j = 0; j < 3; j++)
	{
		cloudNoiseGenerateShader->use();
		cloudNoiseGenerateShader->setInt("includePerlin", false);

		// Render each octave (3 total) for one 8 bit noise texture
		size_t numNoiseOctaves = 3;
		for (size_t k = 0; k < numNoiseOctaves; k++)
		{
			cloudNoiseGenerateShader->setInt("gridSize", (int)channelGridSizes[j * numNoiseOctaves + k]);

			assert(worleyPoints[j * numNoiseOctaves + k].size() <= 1024);
			cloudNoiseInfo.worleyPoints.clear();
			for (size_t worleypointcopy = 0; worleypointcopy < worleyPoints[j * numNoiseOctaves + k].size(); worleypointcopy++)
				cloudNoiseInfo.worleyPoints.push_back(glm::vec4(worleyPoints[j * numNoiseOctaves + k][worleypointcopy], 0.0f));
			glNamedBufferSubData(cloudNoiseUBO, 0, sizeof(glm::vec4) * cloudNoiseInfo.worleyPoints.size(), glm::value_ptr(cloudNoiseInfo.worleyPoints[0]));	 // @NOTE: the offset is sizeof(glm::vec4) bc the memory layout std140 rounds up the 4 bytes of the sizeof(int) up to 16 bytes (size of a vec4)

			//
			// Render each layer with the noise profile
			//
			for (size_t i = 0; i < cloudNoiseTex2Size; i++)
			{
				cloudNoiseGenerateShader->setFloat("currentRenderDepth", (float)i / (float)cloudNoiseTex2Size);

				glNamedFramebufferTextureLayer(noiseFBO, GL_COLOR_ATTACHMENT0, cloudNoise2FractalOctaves[k]->getHandle(), 0, i);
				auto status = glCheckNamedFramebufferStatus(noiseFBO, GL_FRAMEBUFFER);
				if (status != GL_FRAMEBUFFER_COMPLETE)
					std::cout << "Framebuffer not complete! (Worley Noise FBO (pt2)) (Layer: " << i << ") (Channel: " << j << ") (Octave: " << k << ")" << std::endl;

				glBindFramebuffer(GL_FRAMEBUFFER, noiseFBO);
				glClear(GL_COLOR_BUFFER_BIT);
				renderQuad();
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		}

		//
		// Combine octaves into a single noise channel
		//
		for (size_t i = 0; i < cloudNoiseTex2Size; i++)
		{
			cloudNoiseFractalShader->use();
			cloudNoiseFractalShader->setFloat("currentRenderDepth", (float)i / (float)cloudNoiseTex2Size);
			cloudNoiseFractalShader->setSampler("sample1", cloudNoise2FractalOctaves[0]->getHandle());
			cloudNoiseFractalShader->setSampler("sample2", cloudNoise2FractalOctaves[1]->getHandle());
			cloudNoiseFractalShader->setSampler("sample3", cloudNoise2FractalOctaves[2]->getHandle());
			glNamedFramebufferTextureLayer(noiseFBO, GL_COLOR_ATTACHMENT0, cloudNoise2Channels[j]->getHandle(), 0, i);
			auto status = glCheckNamedFramebufferStatus(noiseFBO, GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "Framebuffer not complete! (Worley Noise FBO (pt2)) (Layer: " << i << ") (Channel: " << j << ")" << std::endl;

			glBindFramebuffer(GL_FRAMEBUFFER, noiseFBO);
			glClear(GL_COLOR_BUFFER_BIT);
			renderQuad();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	//
	// Render all 3 of the noise textures onto a single RGB texture
	//
	for (size_t i = 0; i < cloudNoiseTex2Size; i++)
	{
		cloudNoiseCombineShader->use();
		cloudNoiseCombineShader->setFloat("currentRenderDepth", (float)i / (float)cloudNoiseTex2Size);
		cloudNoiseCombineShader->setSampler("R", cloudNoise2Channels[0]->getHandle());
		cloudNoiseCombineShader->setSampler("G", cloudNoise2Channels[1]->getHandle());
		cloudNoiseCombineShader->setSampler("B", cloudNoise2Channels[2]->getHandle());

		glNamedFramebufferTextureLayer(noiseFBO, GL_COLOR_ATTACHMENT0, cloudNoise2->getHandle(), 0, i);
		auto status = glCheckNamedFramebufferStatus(noiseFBO, GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete! (Worley Noise FBO (pt2)) (Layer: " << i << ") (Channel: COMBINER)" << std::endl;

		glBindFramebuffer(GL_FRAMEBUFFER, noiseFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		renderQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// Cleanup
	glDeleteFramebuffers(1, &noiseFBO);
	glDeleteBuffers(1, &cloudNoiseUBO);
}

void RenderManager::destroyCloudNoise()
{
	delete cloudNoise1;
	delete cloudNoise2;
}

void RenderManager::createShaderPrograms()
{
	skybox_program_id = (Shader*)Resources::getResource("shader;skybox");
	debug_csm_program_id = (Shader*)Resources::getResource("shader;debugCSM");
	debug_cloud_noise_program_id = (Shader*)Resources::getResource("shader;debugCloudNoise");
	text_program_id = (Shader*)Resources::getResource("shader;text");
	irradiance_program_id = (Shader*)Resources::getResource("shader;irradianceGeneration");
	prefilter_program_id = (Shader*)Resources::getResource("shader;pbrPrefilterGeneration");
	brdf_program_id = (Shader*)Resources::getResource("shader;brdfGeneration");
	hdrLuminanceProgramId = (Shader*)Resources::getResource("shader;luminance_postprocessing");
	hdrLumAdaptationComputeProgramId = (Shader*)Resources::getResource("shader;computeLuminanceAdaptation");
	bloom_postprocessing_program_id = (Shader*)Resources::getResource("shader;bloom_postprocessing");
	postprocessing_program_id = (Shader*)Resources::getResource("shader;postprocessing");
	pbrShaderProgramId = (Shader*)Resources::getResource("shader;pbr");
	notificationUIProgramId = (Shader*)Resources::getResource("shader;notificationUI");
	INTERNALzPassShader = (Shader*)Resources::getResource("shader;zPassShader");
	ssaoProgramId = (Shader*)Resources::getResource("shader;ssao");
	volumetricProgramId = (Shader*)Resources::getResource("shader;volumetricLighting");
	blurXProgramId = (Shader*)Resources::getResource("shader;blurX");
	blurYProgramId = (Shader*)Resources::getResource("shader;blurY");
	cloudNoiseGenerateShader = (Shader*)Resources::getResource("shader;cloudNoiseGenerate");
	cloudNoiseFractalShader = (Shader*)Resources::getResource("shader;cloudNoiseFractal");
	cloudNoiseCombineShader = (Shader*)Resources::getResource("shader;cloudNoiseCombine");
	cloudEffectShader = (Shader*)Resources::getResource("shader;cloudEffectSS");
}

void RenderManager::destroyShaderPrograms()
{
	Resources::unloadResource("shader;skybox");
	Resources::unloadResource("shader;debugCSM");
	Resources::unloadResource("shader;debugCloudNoise");
	Resources::unloadResource("shader;text");
	Resources::unloadResource("shader;irradianceGeneration");
	Resources::unloadResource("shader;pbrPrefilterGeneration");
	Resources::unloadResource("shader;brdfGeneration");
	Resources::unloadResource("shader;luminance_postprocessing");
	Resources::unloadResource("shader;computeLuminanceAdaptation");
	Resources::unloadResource("shader;bloom_postprocessing");
	Resources::unloadResource("shader;postprocessing");
	Resources::unloadResource("shader;pbr");
	Resources::unloadResource("shader;notificationUI");
	Resources::unloadResource("shader;zPassShader");
	Resources::unloadResource("shader;ssao");
	Resources::unloadResource("shader;volumetricLighting");
	Resources::unloadResource("shader;blurX");
	Resources::unloadResource("shader;blurY");
	Resources::unloadResource("shader;cloudNoiseGenerate");
	Resources::unloadResource("shader;cloudNoiseFractal");
	Resources::unloadResource("shader;cloudNoiseCombine");
	Resources::unloadResource("shader;cloudEffectSS");
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
	if (FT_New_Face(ft, "res/font/arial.ttf", 0, &face))
	{
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
		return;
	}

	FT_Set_Pixel_Sizes(face, 0, 32);

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
			(unsigned int)face->glyph->advance.x,
			(unsigned int)face->glyph->advance.y
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

void RenderManager::destroyFonts()
{
	// Destroy all the font textures
	for (auto it = characters.begin(); it != characters.end(); it++)
	{
		glDeleteTextures(1, &it->second.textureId);
	}

	glDeleteBuffers(1, &textVBO);
	glDeleteVertexArrays(1, &textVAO);
}

void RenderManager::render()
{
#ifdef _DEVELOP
	// @Debug: reload shaders
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
		updateCameraInfoUBO(cameraProjection, cameraView);
	}

#ifdef _DEVELOP
	renderImGuiPass();
#endif

	//
	// Render shadow map(s) to depth framebuffer(s)
	//
	for (size_t i = 0; i < MainLoop::getInstance().lightObjects.size(); i++)
	{
		if (!MainLoop::getInstance().lightObjects[i]->castsShadows)
			continue;

		MainLoop::getInstance().lightObjects[i]->renderPassShadowMap();
	}

	setupSceneShadows();

#ifdef _DEVELOP
	//
	// Render Picking texture
	//
	if (DEBUGdoPicking)
	{
		if (!MainLoop::getInstance().playMode)		// NOTE: no reason in particular for making this !playmode only
		{
			static int mostRecentPickedIndex = -1;

			// Render out picking data
			glViewport(0, 0, (GLsizei)MainLoop::getInstance().camera.width, (GLsizei)MainLoop::getInstance().camera.height);
			glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			pickingRenderFormatShader = (Shader*)Resources::getResource("shader;pickingRenderFormat");
			pickingRenderFormatShader->use();
			for (uint32_t i = 0; i < (uint32_t)MainLoop::getInstance().objects.size(); i++)
			{
				if (i == (uint32_t)mostRecentPickedIndex)
					continue;

				RenderComponent* rc = MainLoop::getInstance().objects[i]->getRenderComponent();
				if (rc == nullptr)
					continue;

				pickingRenderFormatShader->setUint("objectID", i + 1);
				rc->renderShadow(pickingRenderFormatShader);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// Read pixel
			double mouseX, mouseY;
			glfwGetCursorPos(MainLoop::getInstance().window, &mouseX, &mouseY);
			PixelInfo pixInfo = readPixelFromPickingBuffer((uint32_t)mouseX, (uint32_t)(MainLoop::getInstance().camera.height - mouseY - 1));
			size_t id = (size_t)pixInfo.objectID;

			mostRecentPickedIndex = (int)id - 1;

			if (!glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_LEFT_CONTROL) &&
				!glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_RIGHT_CONTROL))
				deselectAllSelectedObject();
			if (mostRecentPickedIndex >= 0)
			{
				if (isObjectSelected((size_t)mostRecentPickedIndex))
					deselectObject((size_t)mostRecentPickedIndex);
				else
					addSelectObject((size_t)mostRecentPickedIndex);
			}
		}

		// Unset flag
		DEBUGdoPicking = false;
	}
#endif

	updateLightInformationUBO();
	renderScene();

	//
	// Compute volumetric lighting for just main light (directional light)
	//
	LightComponent* mainlight = nullptr;
	for (size_t i = 0; i < MainLoop::getInstance().lightObjects.size(); i++)
	{
		if (MainLoop::getInstance().lightObjects[i]->lightType == LightType::DIRECTIONAL)
		{
			mainlight = MainLoop::getInstance().lightObjects[i];
			assert(mainlight->castsShadows);		// NOTE: turn off volumetric lighting if shadows are turned off
			break;
		}
	}
	assert(mainlight != nullptr);		// NOTE: turn off volumetric lighting if shadows are turned off

	if (mainlight->colorIntensity > 0.0f)
	{

		glViewport(0, 0, volumetricTextureWidth, volumetricTextureHeight);
		glBindFramebuffer(GL_FRAMEBUFFER, volumetricFBO);
		volumetricProgramId->use();
		volumetricProgramId->setVec3("mainCameraPosition", MainLoop::getInstance().camera.position);
		volumetricProgramId->setVec3("mainlightDirection", mainlight->facingDirection);
		volumetricProgramId->setMat4("inverseProjectionMatrix", glm::inverse(cameraInfo.projection));
		volumetricProgramId->setMat4("inverseViewMatrix", glm::inverse(cameraInfo.view));
		renderQuad();

		//
		// Blur volumetric lighting pass
		//
		glBindFramebuffer(GL_FRAMEBUFFER, volumetricBlurFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		blurXProgramId->use();
		blurXProgramId->setSampler("textureMap", volumetricTexture->getHandle());
		renderQuad();

		glBindFramebuffer(GL_FRAMEBUFFER, volumetricFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		blurYProgramId->use();
		blurYProgramId->setSampler("textureMap", volumetricBlurTexture->getHandle());
		renderQuad();
	}

	//
	// Do luminance
	//
	glViewport(0, 0, luminanceTextureSize, luminanceTextureSize);
	glBindFramebuffer(GL_FRAMEBUFFER, hdrLumFBO);
	glClear(GL_COLOR_BUFFER_BIT);
	hdrLuminanceProgramId->use();
	hdrLuminanceProgramId->setSampler("hdrColorBuffer", hdrColorBuffer);
	hdrLuminanceProgramId->setSampler("volumetricLighting", volumetricTexture->getHandle());
	hdrLuminanceProgramId->setSampler("cloudEffect", cloudEffectTexture->getHandle());
	hdrLuminanceProgramId->setVec3("sunLightColor", mainlight->color * mainlight->colorIntensity * volumetricLightingStrength * volumetricLightingStrengthExternal);
	renderQuad();
	glGenerateTextureMipmap(hdrLumDownsampling->getHandle());		// This gets the FBO's luminance down to 1x1

	//
	// Kick off light adaptation compute shader
	//
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	hdrLumAdaptationComputeProgramId->use();
	glBindImageTexture(0, hdrLumAdaptationPrevious->getHandle(), 0, GL_TRUE, 0, GL_READ_ONLY, GL_R16F);
	glBindImageTexture(1, hdrLumAdaptation1x1, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R16F);
	glBindImageTexture(2, hdrLumAdaptationProcessed->getHandle(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R16F);
	//constexpr glm::vec2 adaptationSpeeds(0.5f, 2.5f);
	constexpr glm::vec2 adaptationSpeeds(1.5f, 2.5f);
	hdrLumAdaptationComputeProgramId->setVec2("adaptationSpeed", adaptationSpeeds* MainLoop::getInstance().deltaTime);
	glDispatchCompute(1, 1, 1);
	//glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);		// See this later

#ifdef _DEVELOP
	//
	// @Debug: Render wireframe of selected object
	//
	static float selectedColorIntensityTime = -1.15f;
	static size_t prevNumSelectedObjs = 0;
	if (glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE &&		// @NOTE: when pressing shift, this is the start to doing group append for voxel groups, so in the even that this happens, now the creator can see better by not rendering the wireframe (I believe)
		!MainLoop::getInstance().playMode)														// @NOTE: don't render the wireframe if in playmode. It's annoying.
	{
		if (prevNumSelectedObjs != selectedObjectIndices.size())
		{
			// Reset @Copypasta
			prevNumSelectedObjs = selectedObjectIndices.size();
			selectedColorIntensityTime = -1.15f;
		}

		auto objs = getSelectedObjects();
		if (!objs.empty())
		{
			// Render that selected objects!!!!
			Shader* selectionWireframeShader =
				(Shader*)Resources::getResource("shader;selectionSkinnedWireframe");
			selectionWireframeShader->use();

			glDepthMask(GL_FALSE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			{
				glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
				glViewport(0, 0, MainLoop::getInstance().camera.width, MainLoop::getInstance().camera.height);

				for (size_t i = 0; i < objs.size(); i++)
				{
					RenderComponent* rc = objs[i]->getRenderComponent();
					if (rc != nullptr)
					{
						float evaluatedIntensityValue = (std::sinf(selectedColorIntensityTime) + 1);
						//std::cout << evaluatedIntensityValue << std::endl;		@DEBUG
						selectionWireframeShader->setVec4("color", { 0.973f, 0.29f, 1.0f, std::clamp(evaluatedIntensityValue, 0.0f, 1.0f) });
						selectionWireframeShader->setFloat("colorIntensity", evaluatedIntensityValue);
						rc->renderShadow(selectionWireframeShader);
					}
				}
			}
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDepthMask(GL_TRUE);

			selectedColorIntensityTime += MainLoop::getInstance().deltaTime * 4.0f;
		}
	}
	else
	{
		// Reset @Copypasta
		prevNumSelectedObjs = selectedObjectIndices.size();
		selectedColorIntensityTime = -1.15f;
	}
#endif

	//
	// Render ui
	// @NOTE: the cameraUBO changes from the normal world space camera to the UI space camera mat4's.
	// @NOTE: this change does not effect the post processing. Simply just the renderUI contents.
	//
	renderUI();

	//
	// Do bloom: breakdown-preprocessing
	//
	bool firstcopy = true;
	float downscaledFactor = 4.0f;						// NOTE: bloom starts at 1/4 the size
	for (size_t i = 0; i < bloomBufferCount / 2; i++)
	{
		for (size_t j = 0; j < 3; j++)																		// There are three stages in each pass: 1: copy, 2: horiz gauss, 3: vert gauss
		{
			size_t bloomFBOIndex = i * 2 + j % 2;					// Needs to have a sequence of i(0):  0,1,0; i(1): 2,3,2; i(2): 4,5,4; i(3): 6,7,6; i(4): 8,9,8
			size_t colorBufferIndex = i * 2 - 1 + j;				// Needs to have a sequence of i(0): -1,0,1; i(1): 1,2,3; i(2): 3,4,5; i(3): 5,6,7; i(4): 7,8,9
			GLint stageNumber = (GLint)j + 1;						// Needs to have a sequence of i(n):  1,2,3

			glBindFramebuffer(GL_FRAMEBUFFER, bloomFBOs[bloomFBOIndex]);
			glClear(GL_COLOR_BUFFER_BIT);
			bloom_postprocessing_program_id->use();
			bloom_postprocessing_program_id->setSampler("hdrColorBuffer", firstcopy ? hdrColorBuffer : bloomColorBuffers[colorBufferIndex]);
			bloom_postprocessing_program_id->setInt("stage", stageNumber);
			bloom_postprocessing_program_id->setInt("firstcopy", firstcopy);
			bloom_postprocessing_program_id->setFloat("downscaledFactor", downscaledFactor);
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
		bloom_postprocessing_program_id->use();
		bloom_postprocessing_program_id->setSampler("hdrColorBuffer", bloomColorBuffers[colorBufferIndex]);
		bloom_postprocessing_program_id->setSampler("smallerReconstructHDRColorBuffer", bloomColorBuffers[smallerColorBufferIndex]);
		bloom_postprocessing_program_id->setInt("stage", 4);
		bloom_postprocessing_program_id->setInt("firstcopy", firstcopy);
		bloom_postprocessing_program_id->setFloat("downscaledFactor", downscaledFactor);
		renderQuad();

		firstReconstruction = false;
	}

	// Check to make sure the luminance adaptation compute shader is fimished
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

	//
	// Do tonemapping and post-processing
	// with the fbo and render to a quad
	//
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	postprocessing_program_id->use();
	postprocessing_program_id->setSampler("hdrColorBuffer", hdrColorBuffer);
	postprocessing_program_id->setSampler("bloomColorBuffer", bloomColorBuffers[1]);	// 1 is the final color buffer of the reconstructed bloom
	postprocessing_program_id->setSampler("luminanceProcessed", hdrLumAdaptationProcessed->getHandle());
	postprocessing_program_id->setSampler("volumetricLighting", volumetricTexture->getHandle());
	postprocessing_program_id->setSampler("cloudEffect", cloudEffectTexture->getHandle());
	postprocessing_program_id->setVec3("sunLightColor", mainlight->color* mainlight->colorIntensity* volumetricLightingStrength* volumetricLightingStrengthExternal);
	postprocessing_program_id->setFloat("exposure", exposure);
	postprocessing_program_id->setFloat("bloomIntensity", bloomIntensity);
	renderQuad();

	// Swap the hdrLumAdaptation ping-pong textures
	std::swap(hdrLumAdaptationPrevious, hdrLumAdaptationProcessed);

#ifdef _DEVELOP
	// ImGui buffer swap
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
}


void RenderManager::createCameraInfoUBO()
{
	glCreateBuffers(1, &cameraInfoUBO);
	glNamedBufferData(cameraInfoUBO, sizeof(CameraInformation), nullptr, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 3, cameraInfoUBO);
}


void RenderManager::updateCameraInfoUBO(glm::mat4 cameraProjection, glm::mat4 cameraView)
{
	cameraInfo.projection = cameraProjection;
	cameraInfo.view = cameraView;
	cameraInfo.projectionView = cameraProjection * cameraView;
	glNamedBufferSubData(cameraInfoUBO, 0, sizeof(CameraInformation), &cameraInfo);
}


void RenderManager::destroyCameraInfoUBO()
{
	glDeleteBuffers(1, &cameraInfoUBO);
}


void RenderManager::setupSceneShadows()
{
	const size_t numLights = std::min((size_t)RenderLightInformation::MAX_LIGHTS, MainLoop::getInstance().lightObjects.size());
	size_t numShadows = 0;		// NOTE: exclude the csm since it's its own EXT
	bool setupCSM = false;		// NOTE: unfortunately, I can't figure out a way to have multiple directional light csm's, so here's to just one!

	for (size_t i = 0; i < numLights; i++)
	{
		if (!MainLoop::getInstance().lightObjects[i]->castsShadows)
			continue;

		if (MainLoop::getInstance().lightObjects[i]->lightType == LightType::DIRECTIONAL)
		{
			if (!setupCSM)
			{
				ShaderExtCSM_shadow::csmShadowMap = MainLoop::getInstance().lightObjects[i]->shadowMapTexture;
				ShaderExtCSM_shadow::cascadePlaneDistances = ((DirectionalLightLight*)MainLoop::getInstance().lightObjects[i])->shadowCascadeLevels;
				ShaderExtCSM_shadow::cascadeShadowMapTexelSize = ((DirectionalLightLight*)MainLoop::getInstance().lightObjects[i])->shadowCascadeTexelSize;
				ShaderExtCSM_shadow::cascadeCount = (GLint)((DirectionalLightLight*)MainLoop::getInstance().lightObjects[i])->shadowCascadeLevels.size();
				ShaderExtCSM_shadow::nearPlane = MainLoop::getInstance().camera.zNear;
				ShaderExtCSM_shadow::farPlane = MainLoop::getInstance().lightObjects[i]->shadowFarPlane;
			}

			setupCSM = true;
		}
		else if (MainLoop::getInstance().lightObjects[i]->lightType == LightType::SPOT)
		{
			ShaderExtShadow::spotLightShadows[numShadows] = MainLoop::getInstance().lightObjects[i]->shadowMapTexture;

			numShadows++;
		}
		else if (MainLoop::getInstance().lightObjects[i]->lightType == LightType::POINT)
		{
			ShaderExtShadow::pointLightShadows[numShadows] = MainLoop::getInstance().lightObjects[i]->shadowMapTexture;
			ShaderExtShadow::pointLightShadowFarPlanes[numShadows] = ((PointLightLight*)MainLoop::getInstance().lightObjects[i])->farPlane;

			numShadows++;
		}

		// Break out early
		if (numShadows >= ShaderExtShadow::MAX_SHADOWS && setupCSM)
			break;
	}
}


void RenderManager::renderScene()
{
	if (isWireFrameMode)	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	//
	// Z-PASS and RENDER QUEUE SORTING
	//
	glViewport(0, 0, (GLsizei)MainLoop::getInstance().camera.width, (GLsizei)MainLoop::getInstance().camera.height);
	glBindFramebuffer(GL_FRAMEBUFFER, zPrePassFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	INTERNALzPassShader->use();
	ViewFrustum cookedViewFrustum = ViewFrustum::createFrustumFromCamera(MainLoop::getInstance().camera);		// @Optimize: this can be optimized via a mat4 that just changes the initial view frustum
	for (unsigned int i = 0; i < MainLoop::getInstance().renderObjects.size(); i++)
	{
		// NOTE: viewfrustum culling is handled at the mesh level with some magic. Peek in if ya wanna. -Timo
		MainLoop::getInstance().renderObjects[i]->render(&cookedViewFrustum, INTERNALzPassShader);
	}

	//
	// Capture z-passed screen for SSAO
	//
	glViewport(0, 0, (GLsizei)MainLoop::getInstance().camera.width, (GLsizei)MainLoop::getInstance().camera.height);  // ssaoFBOSize, ssaoFBOSize);
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	glClear(GL_COLOR_BUFFER_BIT);
	ssaoProgramId->use();
	ssaoProgramId->setSampler("rotationTexture", ssaoRotationTexture->getHandle());
	ssaoProgramId->setVec2("fullResolution", { MainLoop::getInstance().camera.width, MainLoop::getInstance().camera.height });
	ssaoProgramId->setVec2("invFullResolution", { 1.0f / MainLoop::getInstance().camera.width, 1.0f / MainLoop::getInstance().camera.height });
	ssaoProgramId->setFloat("cameraFOV", MainLoop::getInstance().camera.fov);
	ssaoProgramId->setFloat("zNear", MainLoop::getInstance().camera.zNear);
	ssaoProgramId->setFloat("zNear", MainLoop::getInstance().camera.zNear);
	ssaoProgramId->setFloat("zFar", MainLoop::getInstance().camera.zFar);
	ssaoProgramId->setFloat("powExponent", ssaoScale);
	ssaoProgramId->setFloat("radius", ssaoRadius);
	ssaoProgramId->setFloat("bias", ssaoBias);
	glm::vec4 projInfoPerspective = {
		2.0f / (cameraInfo.projection[0][0]),							// (x) * (R - L)/N
		2.0f / (cameraInfo.projection[1][1]),							// (y) * (T - B)/N
		-(1.0f - cameraInfo.projection[2][0]) / cameraInfo.projection[0][0],  // L/N
		-(1.0f + cameraInfo.projection[2][1]) / cameraInfo.projection[1][1],  // B/N
	};
	ssaoProgramId->setVec4("projInfo", projInfoPerspective);
	renderQuad();

	//
	// Blur SSAO pass
	//
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
	glClear(GL_COLOR_BUFFER_BIT);
	blurXProgramId->use();
	blurXProgramId->setSampler("textureMap", ssaoTexture->getHandle());
	renderQuad();

	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	glClear(GL_COLOR_BUFFER_BIT);
	blurYProgramId->use();
	blurYProgramId->setSampler("textureMap", ssaoBlurTexture->getHandle());
	renderQuad();

	glBlitNamedFramebuffer(
		zPrePassFBO,
		hdrFBO,
		0, 0, MainLoop::getInstance().camera.width, MainLoop::getInstance().camera.height,
		0, 0, MainLoop::getInstance().camera.width, MainLoop::getInstance().camera.height,
		GL_DEPTH_BUFFER_BIT,
		GL_NEAREST
	);

	//
	// Capture z-passed screen for @Clouds
	// @NOTE: I don't know if this render step should be inside of renderscene() or around where the volumetric lighting occurs...  -Timo
	//
	glViewport(0, 0, (GLsizei)MainLoop::getInstance().camera.width, (GLsizei)MainLoop::getInstance().camera.height);  // ssaoFBOSize, ssaoFBOSize);
	glBindFramebuffer(GL_FRAMEBUFFER, cloudEffectFBO);
	glClear(GL_COLOR_BUFFER_BIT);
	cloudEffectShader->use();
	cloudEffectShader->setMat4("inverseProjectionMatrix", glm::inverse(cameraInfo.projection));
	cloudEffectShader->setMat4("inverseViewMatrix", glm::inverse(cameraInfo.view));
	cloudEffectShader->setVec3("mainCameraPosition", MainLoop::getInstance().camera.position);
	cloudEffectShader->setFloat("cloudLayerY", cloudEffectInfo.cloudLayerY);
	cloudEffectShader->setFloat("cloudLayerThickness", cloudEffectInfo.cloudLayerThickness);
	cloudEffectShader->setFloat("cloudNoiseMainSize", cloudEffectInfo.cloudNoiseMainSize);
	cloudEffectShader->setFloat("cloudNoiseDetailSize", cloudEffectInfo.cloudNoiseDetailSize);
	cloudEffectShader->setFloat("densityOffset", cloudEffectInfo.densityOffset);
	cloudEffectShader->setFloat("densityMultiplier", cloudEffectInfo.densityMultiplier);
	cloudEffectShader->setFloat("densityRequirement", cloudEffectInfo.densityRequirement);
	cloudEffectShader->setFloat("darknessThreshold", cloudEffectInfo.darknessThreshold);
	cloudEffectShader->setFloat("lightAbsorptionTowardsSun", cloudEffectInfo.lightAbsorptionTowardsSun);
	cloudEffectShader->setFloat("lightAbsorptionThroughCloud", cloudEffectInfo.lightAbsorptionThroughCloud);
	cloudEffectShader->setSampler("cloudNoiseTexture", cloudNoise1->getHandle());
	cloudEffectShader->setSampler("cloudNoiseDetailTexture", cloudNoise2->getHandle());
	cloudEffectShader->setFloat("raymarchOffset", cloudEffectInfo.raymarchOffset);
	cloudEffectShader->setFloat("maxCloudscapeRadius", cloudEffectInfo.maxCloudscapeRadius);
	cloudEffectShader->setFloat("maxRaymarchLength", cloudEffectInfo.maxRaymarchLength);
	cloudEffectShader->setVec3("lightColor", sunColorForClouds);
	cloudEffectShader->setVec3("lightDirection", skyboxParams.sunOrientation);
	cloudEffectShader->setVec4("phaseParameters", cloudEffectInfo.phaseParameters);
	renderQuad();

	//
	// Blur @Clouds pass
	//
	if (cloudEffectInfo.doBlurPass)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, cloudEffectBlurFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		blurXProgramId->use();
		blurXProgramId->setSampler("textureMap", cloudEffectTexture->getHandle());
		renderQuad();

		glBindFramebuffer(GL_FRAMEBUFFER, cloudEffectFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		blurYProgramId->use();
		blurYProgramId->setSampler("textureMap", cloudEffectBlurTexture->getHandle());
		renderQuad();
	}

	//
	// Render scene normally
	//
	glBlitNamedFramebuffer(
		zPrePassFBO,
		hdrFBO,
		0, 0, MainLoop::getInstance().camera.width, MainLoop::getInstance().camera.height,
		0, 0, MainLoop::getInstance().camera.width, MainLoop::getInstance().camera.height,
		GL_DEPTH_BUFFER_BIT,
		GL_NEAREST
	);

	glViewport(0, 0, (GLsizei)MainLoop::getInstance().camera.width, (GLsizei)MainLoop::getInstance().camera.height);
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
	glClear(GL_COLOR_BUFFER_BIT);

	//
	// Draw skybox
	//
	glDepthMask(GL_FALSE);

	skybox_program_id->use();
	skybox_program_id->setVec3("sunOrientation", skyboxParams.sunOrientation);
	skybox_program_id->setFloat("sunRadius", skyboxParams.sunRadius);
	skybox_program_id->setFloat("sunAlpha", skyboxParams.sunAlpha);
	skybox_program_id->setVec3("sunColor", skyboxParams.sunColor);
	skybox_program_id->setVec3("skyColor1", skyboxParams.skyColor1);
	skybox_program_id->setVec3("groundColor", skyboxParams.groundColor);
	skybox_program_id->setFloat("sunIntensity", skyboxParams.sunIntensity);
	skybox_program_id->setFloat("globalExposure", skyboxParams.globalExposure);
	skybox_program_id->setFloat("cloudHeight", skyboxParams.cloudHeight);
	skybox_program_id->setFloat("perlinDim", skyboxParams.perlinDim);
	skybox_program_id->setFloat("perlinTime", skyboxParams.perlinTime);
	skybox_program_id->setMat3("nightSkyTransform", skyboxParams.nightSkyTransform);
	skybox_program_id->setMat4("projection", cameraInfo.projection);
	skybox_program_id->setMat4("view", cameraInfo.view);
	skybox_program_id->setSampler("nightSkybox", nightSkybox->getHandle());

	//perlinTime += MainLoop::getInstance().deltaTime;

	renderCube();

	glDepthMask(GL_TRUE);

	//
	// Choose which prefilter and irradiance maps to use
	//
	for (int i = (int)numSkyMaps - 1; i >= 0; i--)
	{
		if (-skyboxParams.sunOrientation.y < std::sinf(glm::radians(preBakedSkyMapAngles[i])))
		{
			whichMap = i;

			if (i + 1 >= numSkyMaps)
				mapInterpolationAmt = 0;
			else
			{
				mapInterpolationAmt =
					1 -
					(-skyboxParams.sunOrientation.y - std::sinf(glm::radians(preBakedSkyMapAngles[i + 1]))) /
					(std::sinf(glm::radians(preBakedSkyMapAngles[i])) - std::sinf(glm::radians(preBakedSkyMapAngles[i + 1])));
			}

			break;
		}
	}

	ShaderExtPBR_daynight_cycle::irradianceMap = irradianceMap[whichMap];
	ShaderExtPBR_daynight_cycle::irradianceMap2 = irradianceMap[std::clamp(whichMap + 1, (size_t)0, numSkyMaps - 1)];
	ShaderExtPBR_daynight_cycle::prefilterMap = prefilterMap[whichMap];
	ShaderExtPBR_daynight_cycle::prefilterMap2 = prefilterMap[std::clamp(whichMap + 1, (size_t)0, numSkyMaps - 1)];
	ShaderExtPBR_daynight_cycle::mapInterpolationAmt = mapInterpolationAmt;

	//
	// Compute how much the prefilter and irradiance maps need to spin their input vectors
	//
	glm::vec3 flatSunOrientation = skyboxParams.sunOrientation;
	flatSunOrientation.y = 0;
	flatSunOrientation = glm::normalize(flatSunOrientation);
	sunSpinAmount = glm::toMat3(glm::quat(flatSunOrientation, glm::vec3(1, 0, 0)));

	ShaderExtPBR_daynight_cycle::sunSpinAmount = sunSpinAmount;

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
	this->repopulateAnimationUBO = true;

	//// Start of main render queues: turn on face culling
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	//glFrontFace(GL_CCW);

	//
	// OPAQUE RENDER QUEUE
	//
	glDepthFunc(GL_EQUAL);		// NOTE: this is so that the Z prepass gets used and only fragments that are actually visible will get rendered
	for (size_t i = 0; i < opaqueRQ.meshesToRender.size(); i++)
	{
		opaqueRQ.meshesToRender[i]->render(opaqueRQ.modelMatrices[i], 0, opaqueRQ.boneMatrixMemAddrs[i], RenderStage::OPAQUE_RENDER_QUEUE);
	}
	opaqueRQ.meshesToRender.clear();
	opaqueRQ.modelMatrices.clear();
	opaqueRQ.boneMatrixMemAddrs.clear();
	glDepthFunc(GL_LEQUAL);

	//
	// TEXT RENDER QUEUE
	// @NOTE: there is no frustum culling for text right now... Probs need its own AABB's like models
	//
	glEnable(GL_BLEND);
	for (size_t i = 0; i < textRQ.textRenderers.size(); i++)
	{
		TextRenderer& tr = *textRQ.textRenderers[i];
		renderText(tr);
	}

	//
	// @TEMP: @REFACTOR: try and render a CLOUD!!@!
	//
	static Shader* cloudShader = (Shader*)Resources::getResource("shader;cloud_billboard");
	static Texture* posVolumeTexture = (Texture*)Resources::getResource("texture;cloudTestPos");
	static Texture* negVolumeTexture = (Texture*)Resources::getResource("texture;cloudTestNeg");
	cloudShader->use();
	cloudShader->setMat4("modelMatrix", glm::translate(glm::mat4(1.0f), glm::vec3(0, 1, 0) - MainLoop::getInstance().camera.position) * glm::inverse(cameraInfo.view));
	cloudShader->setVec3("mainLightDirectionVS", glm::mat3(cameraInfo.view) * skyboxParams.sunOrientation);
	cloudShader->setVec3("mainLightColor", sunColorForClouds);
	cloudShader->setSampler("posVolumeTexture", posVolumeTexture->getHandle());
	cloudShader->setSampler("negVolumeTexture", negVolumeTexture->getHandle());
	renderQuad();

	//
	// TRANSPARENT RENDER QUEUE
	//
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	std::sort(
		transparentRQ.commandingIndices.begin(),
		transparentRQ.commandingIndices.end(),
		[this](const size_t& index1, const size_t& index2)
		{
			if (transparentRQ.distancesToCamera[index1] == transparentRQ.distancesToCamera[index2])
				return transparentRQ.meshesToRender[index1]->getDepthPriority() > transparentRQ.meshesToRender[index2]->getDepthPriority();
			return transparentRQ.distancesToCamera[index1] > transparentRQ.distancesToCamera[index2];
		}
	);

	for (size_t& index : transparentRQ.commandingIndices)
	{
		transparentRQ.meshesToRender[index]->render(transparentRQ.modelMatrices[index], 0, transparentRQ.boneMatrixMemAddrs[index], RenderStage::TRANSPARENT_RENDER_QUEUE);
	}
	transparentRQ.commandingIndices.clear();
	transparentRQ.meshesToRender.clear();
	transparentRQ.modelMatrices.clear();
	transparentRQ.boneMatrixMemAddrs.clear();
	transparentRQ.distancesToCamera.clear();

	glDisable(GL_BLEND);

	// End of main render queues: turn off face culling		@TEMP: for transparent render queue
	glDisable(GL_CULL_FACE);

	if (isWireFrameMode)	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#ifdef _DEVELOP
	//
	// @DEBUG: show grid for imguizmo
	//
	static bool imguizmoIsUsingPrevious = false;

	if (ImGuizmo::IsUsing() && !selectedObjectIndices.empty())
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
		glm::mat4 position = glm::translate(glm::mat4(1.0f), INTERNALselectionSystemAveragePosition);
		glm::mat4 rotation = glm::toMat4(INTERNALselectionSystemLatestOrientation);
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), { 100, 100, 100 });
		constexpr float divisor = 4.0f;
		LvlGridMaterial* gridMaterial = (LvlGridMaterial*)Resources::getResource("material;lvlGridMaterial");

		if (showZGrid)
		{
			gridMaterial->setColor(glm::vec3(0.1, 0.1, 1) * 5);
			gridMaterial->applyTextureUniforms();
			gridMaterial->getShader()->setMat4("modelMatrix", position * rotation * scale);
			renderQuad();
		}

		if (showYGrid)
		{
			gridMaterial->setColor(glm::vec3(0.1, 1, 0.1) * 5);
			gridMaterial->applyTextureUniforms();
			glm::mat4 xRotate = glm::toMat4(glm::quat(glm::radians(glm::vec3(90, 0, 0))));
			gridMaterial->getShader()->setMat4("modelMatrix", position * rotation * xRotate * scale);
			renderQuad();
		}

		if (showXGrid)
		{
			gridMaterial->setColor(glm::vec3(1, 0.1, 0.1) * 5);
			gridMaterial->applyTextureUniforms();
			glm::mat4 zRotate = glm::toMat4(glm::quat(glm::radians(glm::vec3(0, 90, 0))));
			gridMaterial->getShader()->setMat4("modelMatrix", position * rotation * zRotate * scale);
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
		debug_csm_program_id->use();
		debug_csm_program_id->setInt("layer", debugCSMLayerNum);
		debug_csm_program_id->setSampler("depthMap", MainLoop::getInstance().lightObjects[0]->shadowMapTexture);
		renderQuad();
	}

	if (showCloudNoiseView)
	{
		debug_cloud_noise_program_id->use();
		debug_cloud_noise_program_id->setFloat("layer", debugCloudNoiseLayerNum);
		debug_cloud_noise_program_id->setSampler("noiseMap", cloudNoise1->getHandle());  // cloudNoise1Channels[debugCloudNoiseChannel]->getHandle());
		renderQuad();
	}
}


void RenderManager::INTERNALaddMeshToOpaqueRenderQueue(Mesh* mesh, const glm::mat4& modelMatrix, const std::vector<glm::mat4>* boneTransforms)
{
	opaqueRQ.meshesToRender.push_back(mesh);
	opaqueRQ.modelMatrices.push_back(modelMatrix);
	opaqueRQ.boneMatrixMemAddrs.push_back(boneTransforms);
}

void RenderManager::INTERNALaddMeshToTransparentRenderQueue(Mesh* mesh, const glm::mat4& modelMatrix, const std::vector<glm::mat4>* boneTransforms)
{
	transparentRQ.commandingIndices.push_back(transparentRQ.meshesToRender.size());
	transparentRQ.meshesToRender.push_back(mesh);
	transparentRQ.modelMatrices.push_back(modelMatrix);
	transparentRQ.boneMatrixMemAddrs.push_back(boneTransforms);

	float distanceToCamera = (cameraInfo.projectionView * modelMatrix * glm::vec4(mesh->getCenterOfGravity(), 1.0f)).z;
	transparentRQ.distancesToCamera.push_back(distanceToCamera);
}

void RenderManager::renderSceneShadowPass(Shader* shader)
{
	//
	// Render everything
	//
	for (unsigned int i = 0; i < MainLoop::getInstance().renderObjects.size(); i++)
	{
		MainLoop::getInstance().renderObjects[i]->renderShadow(shader);
	}
}


void RenderManager::renderUI()
{
	const float camWidth = MainLoop::getInstance().camera.width;
	const float camHeight = MainLoop::getInstance().camera.height;

	if (camWidth == 0.0f || camHeight == 0.0f)
		return;

	const float referenceScreenHeight = 500.0f;
	const float cameraAspectRatio = camWidth / camHeight;
	const glm::mat4 viewMatrix =
		glm::ortho(
			-referenceScreenHeight * cameraAspectRatio / 2.0f,
			referenceScreenHeight * cameraAspectRatio / 2.0f,
			-referenceScreenHeight / 2.0f,
			referenceScreenHeight / 2.0f,
			-1.0f,
			1.0f
		);
	updateCameraInfoUBO(viewMatrix, glm::mat4(1.0f));

	glDepthMask(GL_FALSE);

	//
	// Update UI Stamina Bar
	//
	{
		GameState::getInstance().updateStaminaDepletionChaser(MainLoop::getInstance().deltaTime);
	}

	//
	// Render Message boxes UI
	//
	{
		glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
		glViewport(0, 0, camWidth, camHeight);
		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_BLEND);

		notificationUIProgramId->setFloat("padding", notifPadding);
		notificationUIProgramId->setVec2("extents", notifExtents);
		notificationUIProgramId->setVec3("color1", notifColor1);
		notificationUIProgramId->setVec3("color2", notifColor2);

		glm::vec2 currentPosition = notifPosition;
		for (int i = (int)notifHoldTimers.size() - 1; i >= 0; i--)
		{
			float& timer = notifHoldTimers[i];
			std::string& message = notifMessages[i];

			timer += MainLoop::getInstance().deltaTime;

			float scale = 0.0f;
			if (timer < notifAnimTime)
			{
				// Lead in
				scale = timer / notifAnimTime;
				scale = glm::pow(1 - scale, 4.0f);
			}
			else if (timer < notifAnimTime + notifHoldTime)
			{
				// Hold
				scale = 0.0f;
			}
			else
			{
				// Ease out
				scale = (timer - notifAnimTime - notifHoldTime) / notifAnimTime;
				scale = glm::pow(scale, 4.0f);
			}

			notificationUIProgramId->setFloat("fadeAlpha", (1.0f - scale) * 0.5f);

			const glm::vec3 translation(currentPosition + notifHidingOffset * scale, 0.0f);
			const glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), translation) * glm::scale(glm::mat4(1.0f), glm::vec3(notifExtents, 1.0f));
			notificationUIProgramId->setMat4("modelMatrix", modelMatrix);

			notificationUIProgramId->use();
			renderQuad();

			TextRenderer tr =
			{
				message,
				glm::translate(glm::mat4(1.0f), translation) * glm::scale(glm::mat4(1.0f), glm::vec3(notifMessageSize)),
				notifColor2,
				TextAlignment::CENTER,
				TextAlignment::CENTER
			};
			renderText(tr);

			currentPosition += notifAdvance * (1.0f - scale);

			// Delete stale messages
			if (notifHoldTimers[i] > notifAnimTime * 2.0f + notifHoldTime)
			{
				// DELETE!
				notifHoldTimers.erase(notifHoldTimers.begin() + i);
				notifMessages.erase(notifMessages.begin() + i);
			}
		}

		glDisable(GL_BLEND);
	}

	glDepthMask(GL_TRUE);
}

// TODO: REMEMBER THIS!!!!
//if (ImGui::GetIO().WantCaptureMouse ||
//	ImGuizmo::IsOver() ||
//	ImGuizmo::IsUsing())
//	return;

#ifdef _DEVELOP
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
	static bool showDemoWindow = true;

	static bool showShadowMap = false;

	//
	// Menu Bar
	//
	{
		if (ImGui::BeginMainMenuBar())
		{
			// Mainmenubar keyboard shortcuts
			GLFWwindow* windowRef = MainLoop::getInstance().window;
			const bool ctrl =
				glfwGetKey(windowRef, GLFW_KEY_LEFT_CONTROL) ||
				glfwGetKey(windowRef, GLFW_KEY_RIGHT_CONTROL);
			const bool shift =
				glfwGetKey(windowRef, GLFW_KEY_LEFT_SHIFT) ||
				glfwGetKey(windowRef, GLFW_KEY_RIGHT_SHIFT);

			static bool didKeyboardShortcutPrev = false;
			bool didKeyboardShortcut = false;
			if (ctrl && glfwGetKey(windowRef, GLFW_KEY_O))
			{
				if (!didKeyboardShortcutPrev)
					FileLoading::getInstance().loadFileWithPrompt(true);
				didKeyboardShortcut = true;
			}
			if (ctrl && !shift && glfwGetKey(windowRef, GLFW_KEY_S))
			{
				if (!didKeyboardShortcutPrev)
					FileLoading::getInstance().saveFile(false);
				didKeyboardShortcut = true;
			}
			if (ctrl && shift && glfwGetKey(windowRef, GLFW_KEY_S))
			{
				if (!didKeyboardShortcutPrev)
					FileLoading::getInstance().saveFile(true);
				didKeyboardShortcut = true;
			}
			didKeyboardShortcutPrev = didKeyboardShortcut;

			//ImGui::begin
			if (ImGui::BeginMenu("File"))
			{
				//ShowExampleMenuFile();
				if (ImGui::MenuItem("Open...", "CTRL+O"))
				{
					FileLoading::getInstance().loadFileWithPrompt(true);
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
				auto objs = getSelectedObjects();
				if (ImGui::MenuItem("Duplicate", "CTRL+D", false, !objs.empty()))
				{
					// NOTE: copypasta
					for (size_t i = 0; i < objs.size(); i++)
					{
						nlohmann::json j = objs[i]->savePropertiesToJson();
						j["baseObject"].erase("guid");
						FileLoading::getInstance().createObjectWithJson(j);
					}

					if (objs.size() == 1)
					{
						// Select the last object if there's only one obj selected.
						deselectAllSelectedObject();
						addSelectObject(MainLoop::getInstance().objects.size() - 1);
					}
					else
						// If there's a group selection, then just deselect everything. It's kinda a hassle to try to select everything i think.
						deselectAllSelectedObject();
				}
				if (ImGui::MenuItem("Delete", "SHIFT+Del", false, !objs.empty()))
				{
					// NOTE: This is copypasta
					deleteAllSelectedObjects();
				}

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Windows"))
			{
				if (ImGui::MenuItem("Analytics Overlay", NULL, &showAnalyticsOverlay)) {}
				if (ImGui::MenuItem("Scene Properties", NULL, &showScenePropterties)) {}
				if (ImGui::MenuItem("Object Selection", NULL, &showObjectSelectionWindow)) {}
				if (ImGui::MenuItem("Loaded Resources", NULL, &showLoadedResourcesWindow)) {}
				if (ImGui::MenuItem("ImGui Demo Window", NULL, &showDemoWindow)) {}

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
	if (showDemoWindow)
		ImGui::ShowDemoWindow(&showDemoWindow);

#ifdef _DEVELOP
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
			physx::PxDebugLine& debugLine = debugLinesCopy[i];
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
				pointsOnScreen[ii].x = ImGui::GetWindowPos().x + pointsOnScreen[ii].x * MainLoop::getInstance().camera.width / 2 + MainLoop::getInstance().camera.width / 2;
				pointsOnScreen[ii].y = ImGui::GetWindowPos().y - pointsOnScreen[ii].y * MainLoop::getInstance().camera.height / 2 + MainLoop::getInstance().camera.height / 2;
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

#ifdef _DEVELOP
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
		if (ImGui::Begin("Scene Object List", &showObjectSelectionWindow))
		{
			ImGui::Combo("##Transform operation combo", &imGuizmoTransformOperation, "Translate\0Rotate\0Scale\0Bounds");
			ImGui::Combo("##Transform mode combo", &imGuizmoTransformMode, "Local\0World");

			GLFWwindow* windowRef = MainLoop::getInstance().window;
			auto objs = getSelectedObjects();
			if (!objs.empty() &&
				!ImGuizmo::IsUsing() &&
				glfwGetMouseButton(windowRef, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
			{
				static bool heldQDownPrev = false;
				if (!heldQDownPrev && glfwGetKey(windowRef, GLFW_KEY_Q))
					imGuizmoTransformMode = (int)!(bool)imGuizmoTransformMode;		// Switch between local and world transformation
				heldQDownPrev = glfwGetKey(windowRef, GLFW_KEY_Q) == GLFW_PRESS;

				if (glfwGetKey(windowRef, GLFW_KEY_W))
					imGuizmoTransformOperation = 0;
				if (glfwGetKey(windowRef, GLFW_KEY_E))
					imGuizmoTransformOperation = 1;
				if (glfwGetKey(windowRef, GLFW_KEY_R))
					imGuizmoTransformOperation = 2;
				if ((glfwGetKey(windowRef, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(windowRef, GLFW_KEY_RIGHT_SHIFT)) &&
					glfwGetKey(windowRef, GLFW_KEY_DELETE))
				{
					deleteAllSelectedObjects();
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
							for (size_t i = 0; i < objs.size(); i++)
							{
								nlohmann::json j = objs[i]->savePropertiesToJson();
								j["baseObject"].erase("guid");
								FileLoading::getInstance().createObjectWithJson(j);
							}

							if (objs.size() == 1)
							{
								// Select the last object if there's only one obj selected.
								deselectAllSelectedObject();
								addSelectObject(MainLoop::getInstance().objects.size() - 1);
							}
							else
								// If there's a group selection, then just deselect everything. It's kinda a hassle to try to select everything i think.
								deselectAllSelectedObject();
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
				int shiftSelectRequest[] = { -1, -1 };
				for (size_t n = 0; n < MainLoop::getInstance().objects.size(); n++)
				{
					const bool isSelected = isObjectSelected(n);
					if (ImGui::Selectable(
						(MainLoop::getInstance().objects[n]->name + "##" + MainLoop::getInstance().objects[n]->guid).c_str(),
						isSelected
					))
					{
						bool isShiftHeld =
							glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_LEFT_SHIFT) ||
							glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_RIGHT_SHIFT);

						if (isShiftHeld)
						{
							if (!isSelected && !selectedObjectIndices.empty())
							{
								shiftSelectRequest[0] = glm::min((size_t)selectedObjectIndices[selectedObjectIndices.size() - 1], n);
								shiftSelectRequest[1] = glm::max((size_t)selectedObjectIndices[selectedObjectIndices.size() - 1], n);
							}
						}
						else
						{
							if (!glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_LEFT_CONTROL) &&
								!glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_RIGHT_CONTROL))
								deselectAllSelectedObject();

							if (isObjectSelected(n))
								deselectObject(n);
							else
								addSelectObject(n);
						}
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndListBox();

				//
				// Finally Execute SHIFT+click group selection
				//
				if (shiftSelectRequest[0] >= 0 && shiftSelectRequest[1] >= 0)
					for (size_t n = (size_t)shiftSelectRequest[0]; n <= (size_t)shiftSelectRequest[1]; n++)
						addSelectObject(n);
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
				if (ImGui::Selectable("Voxel Group"))				newObject = new VoxelGroup();
				if (ImGui::Selectable("Spline Tool"))				newObject = new Spline();

				if (newObject != nullptr)
				{
					if (!glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_LEFT_CONTROL) &&
						!glfwGetKey(MainLoop::getInstance().window, GLFW_KEY_RIGHT_CONTROL))
						deselectAllSelectedObject();
					addSelectObject(MainLoop::getInstance().objects.size() - 1);
				}

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
		if (ImGui::Begin("Scene Properties", &showScenePropterties))
		{
			ImGui::DragFloat("Global Timescale", &MainLoop::getInstance().timeScale);
			ImGui::Checkbox("Show shadowmap view", &showShadowMapView);

			ImGui::DragFloat("Cloud layer Y start", &cloudEffectInfo.cloudLayerY);
			ImGui::DragFloat("Cloud layer thickness", &cloudEffectInfo.cloudLayerThickness);
			ImGui::DragFloat("Cloud layer tile size", &cloudEffectInfo.cloudNoiseMainSize);
			ImGui::DragFloat("Cloud layer detail tile size", &cloudEffectInfo.cloudNoiseDetailSize);
			ImGui::DragFloat("Cloud density offset", &cloudEffectInfo.densityOffset, 0.01f);
			ImGui::DragFloat("Cloud density multiplier", &cloudEffectInfo.densityMultiplier, 0.01f);
			ImGui::DragFloat("Cloud density requirement", &cloudEffectInfo.densityRequirement, 0.01f);
			ImGui::DragFloat("Cloud darkness threshold", &cloudEffectInfo.darknessThreshold, 0.01f);
			ImGui::DragFloat("Cloud absorption (sun)", &cloudEffectInfo.lightAbsorptionTowardsSun, 0.01f);
			ImGui::DragFloat("Cloud absorption (cloud)", &cloudEffectInfo.lightAbsorptionThroughCloud, 0.01f);
			ImGui::DragFloat("Cloud Raymarch offset", &cloudEffectInfo.raymarchOffset, 0.01f);
			ImGui::DragFloat("Cloudscape radius", &cloudEffectInfo.maxCloudscapeRadius);
			ImGui::DragFloat("Cloud max raymarch length", &cloudEffectInfo.maxRaymarchLength);
			ImGui::DragFloat4("Cloud phase Parameters", &cloudEffectInfo.phaseParameters.x);
			ImGui::Checkbox("Cloud do blur pass", &cloudEffectInfo.doBlurPass);

			ImGui::Checkbox("Show Cloud noise view", &showCloudNoiseView);
			if (showCloudNoiseView)
			{
				ImGui::InputInt("Cloud noise channel", &debugCloudNoiseChannel);
				ImGui::DragFloat("Cloud noise view layer", &debugCloudNoiseLayerNum);
			}

			static bool showLuminanceTextures = false;
			ImGui::Checkbox("Show Luminance Texture", &showLuminanceTextures);
			if (showLuminanceTextures)
			{
				ImGui::Image((void*)(intptr_t)hdrLumDownsampling->getHandle(), ImVec2(256, 256));
				ImGui::Image((void*)(intptr_t)hdrLumAdaptation1x1, ImVec2(256, 256));
				ImGui::Image((void*)(intptr_t)hdrLumAdaptationProcessed->getHandle(), ImVec2(256, 256));
			}
			ImGui::Separator();

			static bool showSSAOTexture = false;
			ImGui::Checkbox("Show SSAO Texture", &showSSAOTexture);
			if (showSSAOTexture)
			{
				ImGui::Image((void*)(intptr_t)ssaoTexture->getHandle(), ImVec2(512, 288));
			}

			ImGui::DragFloat("SSAO Scale", &ssaoScale, 0.001f);
			ImGui::DragFloat("SSAO Bias", &ssaoBias, 0.001f);
			ImGui::DragFloat("SSAO Radius", &ssaoRadius, 0.001f);
			ImGui::Separator();

			static bool showBloomProcessingBuffers = false;
			ImGui::Checkbox("Show Bloom preprocessing buffers", &showBloomProcessingBuffers);
			if (showBloomProcessingBuffers)
			{
				static int colBufNum = 0;
				ImGui::InputInt("Color Buffer Index", &colBufNum);
				ImGui::Image((void*)(intptr_t)bloomColorBuffers[colBufNum], ImVec2(512, 288));
			}


			ImGui::Separator();
			ImGui::DragFloat("Volumetric Lighting Strength", &volumetricLightingStrength);
			ImGui::DragFloat("External Volumetric Lighting Strength", &volumetricLightingStrengthExternal);

			ImGui::Separator();
			ImGui::DragFloat("Scene Tonemapping Exposure", &exposure);
			ImGui::DragFloat("Bloom Intensity", &bloomIntensity, 0.05f, 0.0f, 5.0f);

			ImGui::Separator();
			ImGui::DragFloat2("notifExtents", &notifExtents[0]);
			ImGui::DragFloat2("notifPosition", &notifPosition[0]);
			ImGui::DragFloat2("notifAdvance", &notifAdvance[0]);
			ImGui::DragFloat2("notifHidingOffset", &notifHidingOffset[0]);
			ImGui::ColorEdit3("notifColor1", &notifColor1[0]);
			ImGui::ColorEdit3("notifColor2", &notifColor2[0]);
			ImGui::DragFloat("notifMessageSize", &notifMessageSize);
			ImGui::DragFloat("notifAnimTime", &notifAnimTime);
			ImGui::DragFloat("notifHoldTime", &notifHoldTime);

			ImGui::Separator();
			ImGui::Text("Skybox properties");
			ImGui::DragFloat("Time of Day", &GameState::getInstance().dayNightTime, 0.005f);
			ImGui::DragFloat("Time of Day susumu sokudo", &GameState::getInstance().dayNightTimeSpeed, 0.001f);
			ImGui::Checkbox("Is Daynight Time moving", &GameState::getInstance().isDayNightTimeMoving);
			ImGui::DragFloat("Sun Radius", &skyboxParams.sunRadius, 0.01f);
			ImGui::ColorEdit3("sunColor", &skyboxParams.sunColor[0]);
			ImGui::ColorEdit3("skyColor1", &skyboxParams.skyColor1[0]);
			ImGui::ColorEdit3("groundColor", &skyboxParams.groundColor[0]);
			ImGui::DragFloat("Sun Intensity", &skyboxParams.sunIntensity, 0.01f);
			ImGui::DragFloat("Global Exposure", &skyboxParams.globalExposure, 0.01f);
			ImGui::DragFloat("Cloud Height", &skyboxParams.cloudHeight, 0.01f);
			ImGui::DragFloat("perlinDim", &skyboxParams.perlinDim, 0.01f);
			ImGui::DragFloat("perlinTime", &skyboxParams.perlinTime, 0.01f);

			ImGui::DragInt("Which ibl map", (int*)&whichMap, 1.0f, 0, 4);
			ImGui::DragFloat("Map Interpolation amount", &mapInterpolationAmt);
		}
		ImGui::End();
	}

	//
	// Render out the properties panels of selected object
	//
	ImGui::Begin("Selected Object Properties");
	{
		auto objs = getSelectedObjects();
		if (objs.size() == 1)
		{
			BaseObject* obj = objs[0];

			//
			// Property panel stuff that's the same with all objects
			//
			ImGui::Text((obj->name + " -- Properties").c_str());
			ImGui::Text(("GUID: " + obj->guid).c_str());
			ImGui::Separator();

			ImGui::InputText("Name", &obj->name);
			ImGui::Separator();

			glm::mat4& transOriginal = obj->getTransform();
			glm::mat4 transCopy = transOriginal;
			PhysicsUtils::imguiTransformMatrixProps(glm::value_ptr(transCopy));

			// To propogate the transform!!
			if (!PhysicsUtils::epsilonEqualsMatrix(transOriginal, transCopy))
				obj->setTransform(transCopy);

			ImGui::Separator();

			// Other property panel stuff
			obj->imguiPropertyPanel();
		}
		else if (objs.size() > 1)
		{
			ImGui::Text("Multiple objects are currently selected");
		}
		else
		{
			ImGui::Text("No object is currently selected");
		}
	}
	ImGui::End();

	//
	// Loaded resources window
	//
	static int currentSelectLoadedResource = -1;
	if (showLoadedResourcesWindow)
	{
		if (ImGui::Begin("Loaded Resources", &showLoadedResourcesWindow))
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
	// @NOTE: bc of how materials are created, I decided to remove this manager window. Maybe in the future we can have a different one that has insertable textures and whatnot eh.
	//
	if (showMaterialsManager)
	{
		//if (ImGui::Begin("Materials Manager", &showMaterialsManager))
		//{
		//	//
		//	// List all materials
		//	//
		//	//std::map<std::string, void*>::iterator it;
		//	//static std::string selectedKey;
		//	//std::map<std::string, void*>& resMapRef = Resources::getResourceMap();
		//	//if (ImGui::BeginListBox("##listbox Current Materials", ImVec2(300, 25 * ImGui::GetTextLineHeightWithSpacing())))
		//	//{
		//	//	int index = 0;
		//	//	for (it = resMapRef.begin(); it != resMapRef.end(); it++, index++)
		//	//	{
		//	//		const bool isSelected = (currentSelectLoadedResource == index);
		//	//		if (ImGui::Selectable(
		//	//			(it->first).c_str(),
		//	//			isSelected
		//	//		))
		//	//		{
		//	//			selectedKey = it->first;
		//	//			currentSelectLoadedResource = index;
		//	//		}
		//	//
		//	//		// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
		//	//		if (isSelected)
		//	//			ImGui::SetItemDefaultFocus();
		//	//	}
		//	//	ImGui::EndListBox();
		//	//}

		//	//
		//	// Find if a material is being selected
		//	//
		//	Material* materialSelected = nullptr;
		//	std::string materialName;
		//	if (currentSelectLoadedResource != -1)
		//	{
		//		auto iterator = Resources::getResourceMap().begin();
		//		std::advance(iterator, currentSelectLoadedResource);

		//		// Check if starts with "material;"
		//		if (iterator->first.find("material;") == 0)
		//		{
		//			materialName = iterator->first;
		//			materialSelected = (Material*)iterator->second;
		//		}
		//	}

		//	//
		//	// Display material properties!!!
		//	//
		//	if (materialSelected == nullptr)
		//	{
		//		ImGui::Text("No material is currently selected from Loaded Resources");
		//	}
		//	else
		//	{
		//		ImGui::Text((materialName + " -- Properties").c_str());
		//		ImGui::DragFloat2("Tiling", materialSelected->getTilingPtr(), 0.05f);
		//		ImGui::DragFloat2("Offset", materialSelected->getOffsetPtr(), 0.05f);
		//	}
		//}
		//ImGui::End();
	}

	//
	// ImGuizmo (NOTE: keep this at the very end so that imguizmo stuff can be rendered after everything else in the background draw list has been rendered)
	//
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 work_pos = viewport->WorkPos;			// Use work area to avoid menu-bar/task-bar, if any!
	ImVec2 work_size = viewport->WorkSize;

	auto objs = getSelectedObjects();
	if (!objs.empty() && !MainLoop::getInstance().playMode && !tempDisableImGuizmoManipulateForOneFrame)
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

		glm::vec3 averagePosition(0.0f);
		glm::quat latestOrientation;
		if (transOperation == ImGuizmo::OPERATION::SCALE && ImGuizmo::IsUsing())
		{
			// Okay, I thought that this would be the fix for scale... but it's now just super buggy hahahaha
			// I don't really care about scale, so we can keep it like this
			//   -Timo
			averagePosition = INTERNALselectionSystemAveragePosition;
			latestOrientation = INTERNALselectionSystemLatestOrientation;
		}
		else
		{
			for (size_t i = 0; i < objs.size(); i++)
			{
				averagePosition += PhysicsUtils::getPosition(objs[i]->getTransform());
				latestOrientation = PhysicsUtils::getRotation(objs[i]->getTransform());
			}
			averagePosition /= (float)objs.size();
		}

		glm::mat4 tempTrans = glm::translate(glm::mat4(1.0f), averagePosition) * glm::toMat4(latestOrientation);
		glm::mat4 deltaMatrix;
		bool changed =
			ImGuizmo::Manipulate(
				glm::value_ptr(cameraInfo.view),
				glm::value_ptr(cameraInfo.projection),
				transOperation,
				transMode,
				glm::value_ptr(tempTrans),
				glm::value_ptr(deltaMatrix),
				&snapValues.x
			);

		if (changed)
		{
			for (size_t i = 0; i < objs.size(); i++)
				objs[i]->setTransform(deltaMatrix * objs[i]->getTransform());

			INTERNALselectionSystemAveragePosition = averagePosition;
			INTERNALselectionSystemLatestOrientation = latestOrientation;
		}

	}
	tempDisableImGuizmoManipulateForOneFrame = false;

	ImGuizmo::ViewManipulate(glm::value_ptr(cameraInfo.view), 8.0f, ImVec2(work_pos.x + work_size.x - 128, work_pos.y), ImVec2(128, 128), 0x10101010);		// NOTE: because the matrix for the cameraview is calculated, there is nothing that this manipulate function does... sad.
}
#endif

void RenderManager::renderText(TextRenderer& tr)		// @Cleanup: this needs to go in some kind of text utils... it's super useful however, soooooo.
{
	// Activate corresponding render state
	text_program_id->use();
	text_program_id->setVec3("diffuseTint", tr.color);
	text_program_id->setMat4("modelMatrix", tr.modelMatrix);

	// Check to see what the width of the text is
	float x = 0;
	if (tr.horizontalAlign != TextAlignment::LEFT)
	{
		for (std::string::const_iterator c = tr.text.begin(); c != tr.text.end(); c++)
			x += (characters[*c].advanceX >> 6); // @NOTE: the advance of the glyph is the amount to move to get the next glyph. ALSO, bitshift by 6 to get value in pixels (2^6 = 64)
		
		if (tr.horizontalAlign == TextAlignment::CENTER)
			x = -x / 2.0f;		// NOTE: x is total width rn so we're using total_width to calculate the starting point of the glyph drawing.
		else if (tr.horizontalAlign == TextAlignment::RIGHT)
			x = -x;				// NOTE: this means that the start of the text will be -total_width
	}

	// Check to see what the total height of the text is	@TODO: @FIXME: FOR SOME REASON THIS DOESN'T WORK?????!?!??!!?!?
	float y = 0;
	if (tr.verticalAlign != TextAlignment::TOP)
	{
		// @TODO: be able to support multiple lines here... this solution atm only supports one line
		/*for (*/ std::string::const_iterator c = tr.text.begin();// c != tr.text.end(); c++)
			y += (characters[*c].advanceY >> 6);

		if (tr.verticalAlign == TextAlignment::CENTER)
			y = y / 2.0f;		// NOTE: positive bc we scoot negatively here
		else if (tr.verticalAlign == TextAlignment::BOTTOM)
			y = y;				// @TODO: this is redundant. Lol.
	}

	// iterate through all characters
	glBindVertexArray(textVAO);
	std::string::const_iterator c;
	for (c = tr.text.begin(); c != tr.text.end(); c++)
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
		glBindTextureUnit(0, ch.textureId);
		// update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, textVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		x += (ch.advanceX >> 6); // bitshift by 6 to get value in pixels (2^6 = 64)
	}
	glBindVertexArray(0);
}

#ifdef _DEVELOP
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
	glCreateBuffers(1, &skeletalAnimationUBO);
	glNamedBufferData(skeletalAnimationUBO, sizeof(glm::mat4x4) * 100, nullptr, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, skeletalAnimationUBO);
}

void RenderManager::destroySkeletalAnimationUBO()
{
	glDeleteBuffers(1, &skeletalAnimationUBO);
}

void RenderManager::INTERNALupdateSkeletalBonesUBO(const std::vector<glm::mat4>* boneTransforms)
{
	if (!repopulateAnimationUBO && boneTransforms == assignedBoneMatricesMemAddr)
		return;

	assignedBoneMatricesMemAddr = boneTransforms;

	glNamedBufferSubData(skeletalAnimationUBO, 0, sizeof(glm::mat4x4) * boneTransforms->size(), &(*boneTransforms)[0]);
}

void RenderManager::createLightInformationUBO()
{
	glCreateBuffers(1, &lightInformationUBO);
	glNamedBufferData(lightInformationUBO, sizeof(lightInformation), nullptr, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 2, lightInformationUBO);
}

void RenderManager::updateLightInformationUBO()
{
	bool TEMPJOJOJOJOJOJOO = false;

	// Insert lights into the struct for the UBO
	int shadowIndex = 0;
	const size_t numLights = std::min((size_t)RenderLightInformation::MAX_LIGHTS, MainLoop::getInstance().lightObjects.size());
	for (size_t i = 0; i < numLights; i++)
	{
		LightComponent* light = MainLoop::getInstance().lightObjects[i];
		glm::vec3 lightDirection = glm::vec3(0.0f);

		if (light->lightType != LightType::POINT)
			lightDirection = glm::normalize(light->facingDirection);	// NOTE: If there is no direction (magnitude: 0), then that means it's a point light ... Check this first in the shader

		glm::vec4 lightPosition = glm::vec4(glm::vec3(light->baseObject->getTransform()[3]), light->lightType == LightType::DIRECTIONAL ? 0.0f : 1.0f);		// NOTE: when a directional light, position doesn't matter, so that's indicated with the w of the vec4 to be 0
		glm::vec3 lightColorWithIntensity = light->color * light->colorIntensity;

		lightInformation.lightPositions[i] = lightPosition;
		lightInformation.lightDirections[i] = glm::vec4(lightDirection, 0);
		lightInformation.lightColors[i] = glm::vec4(lightColorWithIntensity, 0.0f);

		if (!TEMPJOJOJOJOJOJOO && light->lightType == LightType::DIRECTIONAL)
		{
			// MAIN DIREC LUIGHT
			sunColorForClouds = lightColorWithIntensity;
			TEMPJOJOJOJOJOJOO = true;
		}

		if (shadowIndex < ShaderExtShadow::MAX_SHADOWS && light->castsShadows)
		{
			lightInformation.lightDirections[i].a = 1;
			shadowIndex++;
		}
	}
	lightInformation.viewPosition = glm::vec4(MainLoop::getInstance().camera.position, 0.0f);
	lightInformation.numLightsToRender = glm::ivec4(numLights, 0, 0, 0);

	// Stuff into the UBO
	const GLintptr lightPosOffset =		sizeof(glm::vec4) * RenderLightInformation::MAX_LIGHTS * 0;
	const GLintptr lightDirOffset =		sizeof(glm::vec4) * RenderLightInformation::MAX_LIGHTS * 1;
	const GLintptr lightColOffset =		sizeof(glm::vec4) * RenderLightInformation::MAX_LIGHTS * 2;
	const GLintptr viewPosOffset =		sizeof(glm::vec4) * RenderLightInformation::MAX_LIGHTS * 3;
	const GLintptr numLightsOffset =	sizeof(glm::vec4) * RenderLightInformation::MAX_LIGHTS * 3 + sizeof(glm::vec4);

	glNamedBufferSubData(lightInformationUBO, lightPosOffset, sizeof(glm::vec4) * numLights, &lightInformation.lightPositions[0]);
	glNamedBufferSubData(lightInformationUBO, lightDirOffset, sizeof(glm::vec4) * numLights, &lightInformation.lightDirections[0]);
	glNamedBufferSubData(lightInformationUBO, lightColOffset, sizeof(glm::vec4) * numLights, &lightInformation.lightColors[0]);
	glNamedBufferSubData(lightInformationUBO, viewPosOffset, sizeof(glm::vec4), glm::value_ptr(lightInformation.viewPosition));
	glNamedBufferSubData(lightInformationUBO, numLightsOffset, sizeof(lightInformation.numLightsToRender), &lightInformation.numLightsToRender);
}


void RenderManager::destroyLightInformationUBO()
{
	glDeleteBuffers(1, &lightInformationUBO);
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
