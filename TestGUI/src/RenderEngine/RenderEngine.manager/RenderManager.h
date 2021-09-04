#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include "../RenderEngine.model/RenderEngine.model.animation/Animator.h"
#include "../../BaseObject.h"


#include "../Camera.h"


struct Character
{
	unsigned int textureId;
	glm::ivec2 size;
	glm::ivec2 bearing;
	unsigned int advance;
};


class RenderManager
{
public:
	RenderManager(Camera& camera);
	~RenderManager();

	void render(GLFWwindow* window, Camera& camera, std::vector<LightObject*>& lightObjects, std::vector<RenderObject*>& renderObjects);

private:

	GLuint program_id, skybox_program_id, model_program_id, shadow_program_id, shadow_skinned_program_id, pbr_program_id, text_program_id, hdri_program_id, irradiance_program_id, prefilter_program_id, brdf_program_id;
	GLuint vbo, vao, ebo;
	GLuint skyboxVAO, skyboxVBO;

	GLuint envCubemap, irradianceMap, prefilterMap, brdfLUTTexture;

	GLuint depthMapFBO;
	GLuint depthMapTexture;
	
	int selectedBone = 0;
	float modelScale = 1.0f;
	float deltaTimeMultiplier = 42.0f;

	Model tryModel;
	Animator animator;

	Model pbrModel;
	GLuint pbrAlbedoTexture, pbrNormalTexture, pbrMetalnessTexture, pbrRoughnessTexture;
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
	int createShader(GLenum type, const char* fname);
	const GLchar* readFile(const char* filename);

	std::map<char, Character> characters;
	GLuint textVAO, textVBO;
	void createFonts();

	void updateMatrices(glm::mat4 lightProjection, glm::mat4 lightView, glm::mat4 cameraProjection, glm::mat4 cameraView);

	void renderImGui(Camera& camera);
	void renderScene(bool shadowVersion, Camera& camera);
	void renderText(unsigned int programId, std::string text, glm::mat4 modelMatrix, glm::mat4 cameraMatrix, glm::vec3 color);

	void finalize();
};
