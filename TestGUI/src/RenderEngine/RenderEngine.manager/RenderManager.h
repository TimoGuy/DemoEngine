#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "../RenderEngine.model/RenderEngine.model.animation/Animator.h"

class RenderManager
{
public:
	RenderManager();
	~RenderManager();

	int run(void);

private:

	GLFWwindow* window;
	GLuint program_id, skybox_program_id, model_program_id, shadow_program_id, shadow_skinned_program_id;
	GLuint vbo, vao, ebo;
	GLuint skyboxVAO, skyboxVBO;

	GLuint depthMapFBO;
	std::vector<GLuint> depthMapTextures;
	
	int selectedBone = 0;
	float modelScale = 1.0f;
	float deltaTimeMultiplier = 42.0f;

	Model tryModel;
	Animator animator;

	glm::vec3 lightPosition;
	glm::vec3 modelPosition;
	glm::vec3 planePosition;
	glm::vec3 modelEulerAngles;

	glm::mat4	lightProjection, lightView,
				cameraProjection, cameraView;

	void initialize();
	void setupViewPort();
	void setupImGui();
	void createWindow(const char* windowName);
	void createProgram();
    void createRect();
	void createCascadingShadowMap();
	void bindShadowCascadeForWriting(unsigned int cascadeIndex);
	void bindShadowCascadeForReading();
	int createShader(GLenum type, const char* fname);
	const GLchar* readFile(const char* filename);

	void updateMatrices(glm::mat4 lightProjection, glm::mat4 lightView, glm::mat4 cameraProjection, glm::mat4 cameraView);

	void renderImGui();
	void renderScene(bool shadowVersion);

	void finalize();
};
