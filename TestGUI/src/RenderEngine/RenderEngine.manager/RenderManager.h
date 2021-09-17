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


struct TextCharacter
{
	unsigned int textureId;
	glm::ivec2 size;
	glm::ivec2 bearing;
	unsigned int advance;
};


class RenderManager
{
public:
	RenderManager();
	~RenderManager();

	void render();
	void renderScene();
	void renderSceneShadowPass(GLuint shaderProgramId);

	int debugNum = 0;
	void requestSelectObject(ImGuiComponent* imguiObject);

private:
	GLuint program_id, skybox_program_id, model_program_id, shadow_program_id, cascaded_shadow_program_id, debug_csm_program_id, text_program_id, hdri_program_id, irradiance_program_id, prefilter_program_id, brdf_program_id;
	GLuint vbo, vao, ebo;

	GLuint envCubemap, irradianceMap, prefilterMap, brdfLUTTexture;
	
	int selectedBone = 0;
	float modelScale = 1.0f;
	float deltaTimeMultiplier = 42.0f;

	//Model pbrModel;
	glm::vec3 pbrModelPosition, pbrModelScale;

	GLuint hdrTexture;

	glm::vec3 lightPosition;
	glm::vec3 modelPosition;
	glm::vec3 planePosition;
	glm::vec3 modelEulerAngles;
	glm::vec3 textPosition;

	glm::mat4	lightProjection, lightView,
				cameraProjection, cameraView;

	float lightOrthoExtent = 10.0f;
	glm::vec2 lightOrthoZNearFar = glm::vec2(1.0f, 20.0f);		// This is about the limit for a first cascade before things start to disappear

	void createProgram();
    void createRect();
	void createShadowMap();

	std::map<char, TextCharacter> characters;
	GLuint textVAO, textVBO;
	void createFonts();

	void updateMatrices(glm::mat4 lightProjection, glm::mat4 lightView, glm::mat4 cameraProjection, glm::mat4 cameraView);

	void renderImGuiPass();
	void renderImGuiContents();
	void renderText(unsigned int programId, std::string text, glm::mat4 modelMatrix, glm::mat4 cameraMatrix, glm::vec3 color);

	void finalize();
};
