#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>

#include "../../Objects/BaseObject.h"

#include "../RenderEngine.camera/Camera.h"


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
	void requestSelectObject(bool isHoverEvent, ImGuiComponent* imguiObject, PhysicsUtils::RaySegmentHit hitInformation);

	void recreateHDRBuffer();

	int currentSelectedObjectIndex = -1, currentHoveringObjectIndex = -1;

	inline GLuint getIrradianceMap() { return irradianceMap; }
	inline GLuint getPrefilterMap() { return prefilterMap; }
	inline GLuint getBRDFLUTTexture() { return brdfLUTTexture; }

	void setupSceneLights(GLuint programId);

	void updateSkeletalBonesUBO(const std::vector<glm::mat4>* boneTransforms);

	// Transparent rendering
	void INTERNALaddTransparentMeshToDeferRender(Mesh* mesh, const glm::mat4& modelMatrix);

	// @PHYSX_VISUALIZATION
	void physxVisSetDebugLineList(std::vector<physx::PxDebugLine>* lineList);
	static bool renderPhysicsDebug;

private:
	GLuint program_id, skybox_program_id, model_program_id, shadow_program_id, debug_csm_program_id, text_program_id, irradiance_program_id, prefilter_program_id, brdf_program_id, bloom_postprocessing_program_id, postprocessing_program_id, pbrShaderProgramId, hudUIProgramId;

	GLuint hdrFBO, hdrDepthRBO, hdrColorBuffer, hdrPBRGenCaptureFBO, hdrPBRGenCaptureRBO;

	static const uint32_t bloomBufferCount = 7 * 2;			// 7 passes with 2 pingpong buffers each
	GLuint bloomFBOs[bloomBufferCount], bloomColorBuffers[bloomBufferCount];
	float bloomIntensity = 0.005f;

	//float deltaTimeMultiplier = 42.0f;			// @Remember: this is a very important number to multiply the time for the animations.

	GLuint envCubemap, irradianceMap, prefilterMap, brdfLUTTexture;


	glm::mat4 cameraProjection, cameraView;

	void createShaderPrograms();
    void createHDRSkybox(bool first);
	void createHDRBuffer();
	void destroyHDRBuffer();

	std::map<char, TextCharacter> characters;
	GLuint textVAO, textVBO;
	void createFonts();

	void updateMatrices(glm::mat4 cameraProjection, glm::mat4 cameraView);

	// Transparency rendering stuff
	std::vector<size_t> transparentCommandingIndices;
	std::vector<Mesh*> transparentMeshesToRender;
	std::vector<glm::mat4> transparentModelMatrices;
	std::vector<const std::vector<glm::mat4>*> transparentBoneMatrixMemAddrs;
	std::vector<float> transparentDistancesToCamera;

	// ImGui Debug stuff
	bool isWireFrameMode;
	void renderImGuiPass();
	void renderImGuiContents();
	void renderText(unsigned int programId, std::string text, glm::mat4 modelMatrix, glm::mat4 cameraMatrix, glm::vec3 color);

	// @PHYSX_VISUALIZATION
	std::vector<physx::PxDebugLine>* physxVisDebugLines;

	// Skeletal Animation UBO
	GLuint skeletalAnimationUBO;
	void createSkeletalAnimationUBO();
	const std::vector<glm::mat4>* assignedBoneMatricesMemAddr = nullptr;
};
