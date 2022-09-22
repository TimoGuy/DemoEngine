#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
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


struct CloudNoiseInformation
{
	std::vector<glm::vec4> worleyPoints;
};

// Added values from (https://github.com/SebLague/Clouds/blob/fcc997c40d36c7bedf95a294cd2136b8c5127009/Assets/_Scenes/Clouds%20Test.unity)
struct CloudEffectInformation
{
	float cloudLayerY = 92.0f;
	float cloudLayerThickness = 1125.0f;		// @TODO: add cloud point saving. This can be done inside of a json file or something for now eh. Eventually into textures ideally eh.
	float cloudNoiseMainSize = 3721.0f;
	float cloudNoiseDetailSize = 160.0f;
	glm::vec3 cloudNoiseDetailOffset = { 0.0f, 0.0f, 0.0f };
	glm::vec3 cloudNoiseDetailVelocity = { 1.0f, 0.0f, 2.0f };		// @NOTE: good and subtle eh!  -Timo
	float densityOffsetInner = -0.3f;		// @NOTE: Inside the walls where it's cooler
	float densityOffsetOuter = -0.15f;		// @NOTE: Outside the walls, where the fire ppl live
	float densityOffsetChangeRadius = 4000.0f;	// The separation between the inner and outer areas... right where the wall is is where the change happens.
	float densityMultiplier = 0.062f;
	float densityRequirement = 0.0f;
	float darknessThreshold = 0.01f;  // 0.37f;  // 0.07f;
	float irradianceStrength = 1.0f;
	float lightAbsorptionTowardsSun = 0.01f;
	float lightAbsorptionThroughCloud = 0.2f;
	float raymarchOffset = 1.0f;
	glm::vec2 raymarchCascadeLevels = { 275.0f, 6000.0f };
	float farRaymarchStepsizeMultiplier = 1.0f;
	//float maxRaymarchLength = 5000.0f;
	glm::vec4 phaseParameters = { 0.83f, 0.3f, 0.4f, 0.15f };		// @HARDCODE: Forward scattering, Backscattering, BaseBrightness, PhaseFactor
	bool doBlurPass = false;
	float cameraPosJitterScale = 0.2f;
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

	glm::vec3 sunColorForClouds;  // @TEMP: @REFACTOR: this is the saved sunlight intensity and color

	int debugCSMLayerNum = 0;
	int debugCloudNoiseChannel = 0;
	float debugCloudNoiseLayerNum = 0;

	void recreateRenderBuffers();

#ifdef _DEVELOP
	std::vector<size_t> selectedObjectIndices;
	std::vector<BaseObject*> getSelectedObjects();
	bool isObjectSelected(size_t index);
	bool isObjectSelected(const std::string& guid);
	void addSelectObject(size_t index);
	void deselectObject(size_t index);
	void deselectAllSelectedObject();
	void deleteAllSelectedObjects();
	glm::vec3 INTERNALselectionSystemAveragePosition;
	glm::quat INTERNALselectionSystemLatestOrientation;

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
	Shader* debug_csm_program_id, *debug_cloud_noise_program_id, *text_program_id, *irradiance_program_id, *prefilter_program_id, *brdf_program_id, *bloom_postprocessing_program_id, *postprocessing_program_id, *fxaaPostProcessingShader, *pbrShaderProgramId, *notificationUIProgramId;

	GLuint hdrFBO, hdrFXAAFBO, hdrDepthRBO, hdrColorBuffer, hdrFXAAColorBuffer, hdrPBRGenCaptureFBO, hdrPBRGenCaptureRBO;

	// HDR screen luminance adjustment
	Shader* hdrLuminanceProgramId, *hdrLumAdaptationComputeProgramId;
	GLuint hdrLumFBO, hdrLumAdaptation1x1;
	Texture *hdrLumDownsampling, *hdrLumAdaptationPrevious, *hdrLumAdaptationProcessed;
	float exposure = 0.3f;

	// Volumetric lighting
	Shader* volumetricProgramId, *blurXProgramId, *blurYProgramId;
	GLuint volumetricFBO, volumetricBlurFBO;
	Texture* volumetricTexture;
	Texture* volumetricBlurTexture;
	int volumetricTextureWidth, volumetricTextureHeight;
	float volumetricLightingStrength, volumetricLightingStrengthExternal;

	// SSAO effect		(Uses NVIDIA's HBAO effect) (@NOTE: @TODO: I took out the plus (+) from HBAO+... there's no more temporal reprojection bc I didn't understand how it worked lol  -Timo)
	Shader* ssaoProgramId;
	GLuint ssaoFBO, ssaoBlurFBO;
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
	float bloomIntensity = 0.005f;

	// Cloud noises
	GLuint cloudEffectFBO, cloudEffectBlurFBO;
	Texture* cloudEffectTexture, *cloudEffectBlurTexture;
	int cloudEffectTextureWidth, cloudEffectTextureHeight;
	Texture* cloudNoise1;
	Texture* cloudNoise2;
	Shader* cloudNoiseGenerateShader,
		*cloudNoiseFractalShader,
		*cloudNoiseCombineShader,
		*cloudEffectShader,
		*cloudEffectFloodFillShaderX,
		*cloudEffectFloodFillShaderY,
		*cloudEffectColorFloodFillShaderX,
		*cloudEffectColorFloodFillShaderY,
		*simpleDenoiseShader,
		*cloudEffectApplyShader,
		*blurX3ProgramId,
		*blurY3ProgramId,
		*cloudEffectTAAHistoryShader;
	CloudNoiseInformation cloudNoiseInfo;
	CloudEffectInformation cloudEffectInfo;
	bool doCloudDenoiseNontemporal = false;

	// Skybox Rendering
	const static int skyboxDepthSlicedLUTSize = 32;
	const static int skyboxLowResSize = 256;
	const static int irradianceMapSize = 32;
	const static int prefilterMapSize = 128;
	const static unsigned int maxMipLevels = 5;  // glm::floor(glm::log2((float_t)prefilterMapSize)) + 1;
	GLuint skyboxFBO, skyboxBlurFBO, skyboxDepthSlicedLUTFBOs[skyboxDepthSlicedLUTSize], skyboxDetailsSSFBO;
	Texture* skyboxLowResTexture, *skyboxLowResBlurTexture, *skyboxDepthSlicedLUT, *nightSkyboxCubemap, * skyboxDetailsSS;
	Shader* skybox_program_id, *skyboxDetailsShader;
	SkyboxParams skyboxParams;
	static const size_t numSkyMaps = 6;
	GLuint envCubemap, brdfLUTTexture, irradianceMap[numSkyMaps], prefilterMap[numSkyMaps];
	Texture* irradianceMapInterpolated, *prefilterMapInterpolated;
	GLuint irradianceMapInterpolatedFBO, prefilterMapInterpolatedFBO;
	Shader* environmentMapMixerShader;
	float_t preBakedSkyMapAngles[numSkyMaps] = { 90.0f, 10.0f, 5.0f, 0.0f, -10.0f, -30.0f };			// NOTE: these values must be listed in descending order
	size_t whichMap = 0;
	glm::mat3 sunSpinAmount;
	float mapInterpolationAmt;		// @NOTE: this is here only for viewing the value in imgui property panel

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
	void createCloudNoise();
	void destroyCloudNoise();

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
