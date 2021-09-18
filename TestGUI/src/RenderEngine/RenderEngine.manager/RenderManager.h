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
	//GLuint vbo, vao, ebo;

	//float deltaTimeMultiplier = 42.0f;			// @Remember: this is a very important number to multiply the time for the animations.

	GLuint hdrTexture;				// NOTE: kept here for debug purposes in the "scene properties window"
	GLuint envCubemap, irradianceMap, prefilterMap, brdfLUTTexture;

	glm::mat4 cameraProjection, cameraView;

	void createProgram();
    void createRect();
	void createHDRBuffer();

	std::map<char, TextCharacter> characters;
	GLuint textVAO, textVBO;
	void createFonts();

	void updateMatrices(glm::mat4 cameraProjection, glm::mat4 cameraView);

	void renderImGuiPass();
	void renderImGuiContents();
	void renderText(unsigned int programId, std::string text, glm::mat4 modelMatrix, glm::mat4 cameraMatrix, glm::vec3 color);
};
