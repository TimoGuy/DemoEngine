#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>

#include "../../objects/BaseObject.h"

#include "../camera/Camera.h"


class Texture;
typedef unsigned int GLuint;

namespace PhysicsUtils
{
	struct RaySegmentHit;
}

struct TextCharacter
{
	unsigned int textureId;
	glm::ivec2 size;
	glm::ivec2 bearing;
	unsigned int advance;
};

class Mesh;


#ifdef _DEVELOP
struct PixelInfo
{
	float objectID;

	PixelInfo()
	{
		objectID = 0.0f;
	}
};
#endif


struct SkyboxParams
{
	glm::vec3 sunOrientation = { 0, 1, 0 };		// NOTE: this is different from facingDirection of the main directional light, bc this is used for dayNightCycle (See https://www.youtube.com/watch?v=RRR-rI9zzgk)
	float sunRadius = 0.085f;
	float sunAlpha = 1.0f;
	glm::vec3 sunColor{ 1, 1, 1 };
	glm::vec3 skyColor1{ 0.043 , 0.182 ,0.985 };
	glm::vec3 groundColor{ 0.51,0.27, 0.11 };
	float sunIntensity = 25;
	float globalExposure = 1;
	float cloudHeight = -50;
	float perlinDim = 2;
	float perlinTime = 0;
	glm::mat3 nightSkyTransform = glm::mat3(1.0f);
};


struct OpaqueRenderQueue
{
	std::vector<Mesh*> meshesToRender;
	std::vector<glm::mat4> modelMatrices;
	std::vector<const std::vector<glm::mat4>*> boneMatrixMemAddrs;
};


struct TransparentRenderQueue
{
	std::vector<size_t> commandingIndices;
	std::vector<Mesh*> meshesToRender;
	std::vector<glm::mat4> modelMatrices;
	std::vector<const std::vector<glm::mat4>*> boneMatrixMemAddrs;
	std::vector<float> distancesToCamera;
};


struct RenderLightInformation
{
	static constexpr int MAX_LIGHTS = 1024;

	glm::vec4 lightPositions[MAX_LIGHTS];
	glm::vec4 lightDirections[MAX_LIGHTS];		// .a contains if has shadow or not
	glm::vec4 lightColors[MAX_LIGHTS];
	glm::vec4 viewPosition;
	glm::ivec4 numLightsToRender;		// Dumb std140 padding, making all these vec4's
};


class RenderManager
{
public:
	RenderManager();
	~RenderManager();

	void render();
	void renderScene();
	void renderSceneShadowPass(GLuint shaderProgramId);
	void renderUI();


	int debugCSMLayerNum = 0;

	void recreateRenderBuffers();

#ifdef _DEVELOP
	int currentSelectedObjectIndex = -1;
#endif

	inline GLuint getBRDFLUTTexture() { return brdfLUTTexture; }
	inline glm::mat3 getSunSpinAmount() { return sunSpinAmount; }

	inline GLuint getIrradianceMap()
	{
		return irradianceMap[whichMap];
	}
	
	inline GLuint getIrradianceMap2()
	{
		return irradianceMap[std::clamp(whichMap + 1, (size_t)0, numSkyMaps - 1)];
	}
	
	inline GLuint getPrefilterMap()
	{
		return prefilterMap[whichMap];
	}
	
	inline GLuint getPrefilterMap2()
	{
		return prefilterMap[std::clamp(whichMap + 1, (size_t)0, numSkyMaps - 1)];
	}
	
	inline float getMapInterpolationAmt()
	{
		return mapInterpolationAmt;
	}

	void setupSceneShadows(GLuint programId);

	void INTERNALupdateSkeletalBonesUBO(const std::vector<glm::mat4>* boneTransforms);

	// Render Queues
	void INTERNALaddMeshToOpaqueRenderQueue(Mesh* mesh, const glm::mat4& modelMatrix, const std::vector<glm::mat4>* boneTransforms);
	void INTERNALaddMeshToTransparentRenderQueue(Mesh* mesh, const glm::mat4& modelMatrix, const std::vector<glm::mat4>* boneTransforms);

#ifdef _DEVELOP
	// @PHYSX_VISUALIZATION
	void physxVisSetDebugLineList(std::vector<physx::PxDebugLine>* lineList);
#endif

	// @DEBUG: @WHEN_RELEASE: remove these when done with game.
	// This should be in an #ifdef _DEVELOP block...
	static bool isWireFrameMode;
	static bool renderPhysicsDebug;
	static bool renderMeshRenderAABB;

	// Skybox Params handle
	inline SkyboxParams& getSkyboxParams() { return skyboxParams; }

private:
	GLuint debug_csm_program_id, text_program_id, irradiance_program_id, prefilter_program_id, brdf_program_id, bloom_postprocessing_program_id, postprocessing_program_id, pbrShaderProgramId, hudUIProgramId;

	GLuint hdrFBO, hdrDepthRBO, hdrColorBuffer, hdrPBRGenCaptureFBO, hdrPBRGenCaptureRBO;

	GLuint hdrLuminanceProgramId, hdrLumAdaptationComputeProgramId, hdrLumFBO, hdrLumAdaptation1x1;
	Texture *hdrLumDownsampling, *hdrLumAdaptationPrevious, *hdrLumAdaptationProcessed;

	static const uint32_t bloomBufferCount = 7 * 2;			// 7 passes with 2 pingpong buffers each
	GLuint bloomFBOs[bloomBufferCount], bloomColorBuffers[bloomBufferCount];
	float exposure = 0.15f;
	float bloomIntensity = 0.005f;

	//float deltaTimeMultiplier = 42.0f;			// @Remember: this is a very important number to multiply the time for the animations.

	// Skybox Rendering
	GLuint skybox_program_id;
	SkyboxParams skyboxParams;
	static const size_t numSkyMaps = 6;
	GLuint envCubemap, brdfLUTTexture, irradianceMap[numSkyMaps], prefilterMap[numSkyMaps];
	float_t preBakedSkyMapAngles[numSkyMaps] = { 90.0f, 10.0f, 5.0f, 0.0f, -10.0f, -30.0f };			// NOTE: these values must be listed in descending order
	size_t whichMap = 0;
	glm::mat3 sunSpinAmount;
	float mapInterpolationAmt;
	Texture* nightSkybox;


	glm::mat4 cameraProjection, cameraView;

	void createShaderPrograms();
    void createHDRSkybox(bool first, size_t index, const glm::vec3& sunOrientation);
	void createHDRBuffer();
	void destroyHDRBuffer();
	void createLumenAdaptationTextures();


	std::map<char, TextCharacter> characters;
	GLuint textVAO, textVBO;
	void createFonts();

	void updateMatrices(glm::mat4 cameraProjection, glm::mat4 cameraView);

	// Render Queues
	GLuint INTERNALzPassShader;
	GLuint zPrePassFBO;
	Texture* zPrePassDepthTexture;
	OpaqueRenderQueue opaqueRQ;
	TransparentRenderQueue transparentRQ;

#ifdef _DEVELOP
	// ImGui Debug stuff
	void renderImGuiPass();
	void renderImGuiContents();
#endif

	void renderText(unsigned int programId, std::string text, glm::mat4 modelMatrix, glm::mat4 cameraMatrix, glm::vec3 color);

#ifdef _DEVELOP
	// @PHYSX_VISUALIZATION
	std::vector<physx::PxDebugLine>* physxVisDebugLines;
#endif

#ifdef _DEVELOP
	// Picking texture
	void createPickingBuffer();
	void destroyPickingBuffer();
	PixelInfo readPixelFromPickingBuffer(uint32_t x, uint32_t y);

public:
	void flagForPicking() { DEBUGdoPicking = true; }
private:
	bool DEBUGdoPicking = false;

	GLuint pickingRenderFormatProgramId;

	GLuint pickingFBO;
	GLuint pickingRBO;
	GLuint pickingColorBuffer;
#endif

	// Skeletal Animation UBO
	bool repopulateAnimationUBO = true;
	GLuint skeletalAnimationUBO;
	void createSkeletalAnimationUBO();
	const std::vector<glm::mat4>* assignedBoneMatricesMemAddr = nullptr;

	// Light information UBO
	GLuint lightInformationUBO;
	RenderLightInformation lightInformation;
	void createLightInformationUBO();
	void updateLightInformationUBO();
};
