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
class Shader;
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
	unsigned int advanceX;
	unsigned int advanceY;
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
	float sunRadius = 0.15f;
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


struct TextRenderQueue
{
	std::vector<TextRenderer*> textRenderers;
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


struct CameraInformation
{
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 projectionView;
};


class RenderManager
{
public:
	RenderManager();
	~RenderManager();

	void setupSceneShadows();

	void render();
	void renderScene();
	void renderSceneShadowPass(Shader* shader);
	void renderUI();


	int debugCSMLayerNum = 0;

	void recreateRenderBuffers();

#ifdef _DEVELOP
	int currentSelectedObjectIndex = -1;
	bool tempDisableImGuizmoManipulateForOneFrame = false;
#endif

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

	// Volumetric setting
	inline void setVolumetricLightingStrength(float amount) { volumetricLightingStrengthExternal = amount; }

	// Render Text
	void addTextRenderer(TextRenderer* textRenderer);
	void removeTextRenderer(TextRenderer* textRenderer);

	// Message System
	void pushMessage(const std::string& text);

private:
	Shader* debug_csm_program_id, *text_program_id, *irradiance_program_id, *prefilter_program_id, *brdf_program_id, *bloom_postprocessing_program_id, *postprocessing_program_id, *pbrShaderProgramId, *hudUIProgramId, *notificationUIProgramId;

	GLuint hdrFBO, hdrDepthRBO, hdrColorBuffer, hdrPBRGenCaptureFBO, hdrPBRGenCaptureRBO;

	// HDR screen luminance adjustment
	Shader* hdrLuminanceProgramId, *hdrLumAdaptationComputeProgramId;
	GLuint hdrLumFBO, hdrLumAdaptation1x1;
	Texture *hdrLumDownsampling, *hdrLumAdaptationPrevious, *hdrLumAdaptationProcessed;

	// Volumetric lighting
	Shader* volumetricProgramId, *blurXProgramId, *blurYProgramId;
	GLuint volumetricFBO, volumetricBlurFBO;
	Texture* volumetricTexture;
	Texture* volumetricBlurTexture;
	int volumetricTextureWidth, volumetricTextureHeight;
	float volumetricLightingStrength, volumetricLightingStrengthExternal;

	// SSAO effect			// @Deprecate: Looks like the S word
	Shader* ssaoProgramId;
	GLuint ssaoFBO, ssaoBlurFBO;// , blurXProgramId, blurYProgramId;
	float ssaoFBOSize = 1024;
	Texture* ssaoRotationTexture;
	Texture* ssaoTexture;
	Texture* ssaoBlurTexture;
	float ssaoScale = 0.5f;
	float ssaoBias = 0.1f;
	float ssaoRadius = 2.0f;

	// Bloom effect
	static const uint32_t bloomBufferCount = 7 * 2;			// 7 passes with 2 pingpong buffers each
	GLuint bloomFBOs[bloomBufferCount], bloomColorBuffers[bloomBufferCount];
	float exposure = 0.15f;
	float bloomIntensity = 0.005f;

	//float deltaTimeMultiplier = 42.0f;			// @Remember: this is a very important number to multiply the time for the animations.

	// Skybox Rendering
	Shader* skybox_program_id;
	SkyboxParams skyboxParams;
	static const size_t numSkyMaps = 6;
	GLuint envCubemap, brdfLUTTexture, irradianceMap[numSkyMaps], prefilterMap[numSkyMaps];
	float_t preBakedSkyMapAngles[numSkyMaps] = { 90.0f, 10.0f, 5.0f, 0.0f, -10.0f, -30.0f };			// NOTE: these values must be listed in descending order
	size_t whichMap = 0;
	glm::mat3 sunSpinAmount;
	float mapInterpolationAmt;
	Texture* nightSkybox;

	// UI StaminaBar
	void loadResources();
	void unloadResources();
	glm::vec3 staminaBarPosition{ 128, -50, 0 };
	float staminaBarSize = 15.0f;
	glm::vec3 staminaBarColor1{ 0.0588, 0.0588, 0.0588 };
	glm::vec3 staminaBarColor2{ 0.3216, 0.7765, 0.3647 };
	glm::vec3 staminaBarColor3{ 0.1686, 0.4275, 0.1922 };
	glm::vec3 staminaBarColor4{ 0.5804, 0.05098, 0.05098 };
	float staminaBarDepleteColorIntensity = 1024.0f;		// Looks like a lightsaber
	Model* staminaBar;

	// UI Notifications
	float notifPadding = 1.0f;
	glm::vec2 notifExtents{ 82.0f, 24.0f };
	glm::vec2 notifPosition{ 284.0f, 92.0f };
	glm::vec2 notifAdvance{ 0.0f, -54.0f };
	glm::vec2 notifHidingOffset{ 32.0f, 0.0f };
	glm::vec3 notifColor1{ 0.0588, 0.0588, 0.0588 };
	glm::vec3 notifColor2{ 0.6, 0.780392157, 0.61568627451 };
	float notifMessageSize = 0.5f;
	float notifAnimTime = 0.5f;
	float notifHoldTime = 5.0f;
	std::vector<float> notifHoldTimers;
	std::vector<std::string> notifMessages;

	// Camera Information
	GLuint cameraInfoUBO;
	CameraInformation cameraInfo;
	void createCameraInfoUBO();
	void updateCameraInfoUBO(glm::mat4 cameraProjection, glm::mat4 cameraView);
	void destroyCameraInfoUBO();

	void createShaderPrograms();
	void destroyShaderPrograms();
	void createHDRBuffer();
	void destroyHDRBuffer();
	void createLumenAdaptationTextures();
	void destroyLumenAdaptationTextures();

    void createHDRSkybox(bool first, size_t index, const glm::vec3& sunOrientation);


	// Render Queues
	Shader* INTERNALzPassShader;
	GLuint zPrePassFBO;
	Texture* zPrePassDepthTexture;
	//Texture* ssNormalTexture;		@DEPRECATE: normal reconstruction was faster for HBAO, so this'll get the can. // Kinda like a normal map for a g-buffer
	OpaqueRenderQueue opaqueRQ;
	TransparentRenderQueue transparentRQ;
	TextRenderQueue textRQ;

#ifdef _DEVELOP
	// ImGui Debug stuff
	void renderImGuiPass();
	void renderImGuiContents();
#endif

	// Fonts
	std::map<char, TextCharacter> characters;
	GLuint textVAO, textVBO;
	void createFonts();
	void destroyFonts();
	void renderText(TextRenderer& tr);

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

	Shader* pickingRenderFormatShader;

	GLuint pickingFBO;
	GLuint pickingRBO;
	GLuint pickingColorBuffer;
#endif

	// Skeletal Animation UBO
	bool repopulateAnimationUBO = true;
	GLuint skeletalAnimationUBO;
	void createSkeletalAnimationUBO();
	void destroySkeletalAnimationUBO();
	const std::vector<glm::mat4>* assignedBoneMatricesMemAddr = nullptr;

	// Light information UBO
	GLuint lightInformationUBO;
	RenderLightInformation lightInformation;
	void createLightInformationUBO();
	void updateLightInformationUBO();
	void destroyLightInformationUBO();
};
