#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include <string>

#include "../Camera.h"

class RenderManager
{
public:
	RenderManager();
	~RenderManager();

	int run(void);

private:

	GLFWwindow* window;
	GLuint program_id, skybox_program_id, model_program_id;
	GLuint vbo, vao, ebo;
	GLuint skyboxVAO, skyboxVBO;
	int selectedBone = 0;
	float modelScale = 1.0f;
	float deltaTimeMultiplier = 42.0f;

	glm::vec3 lightPosition;
	glm::vec3 modelPosition;
	glm::vec3 modelEulerAngles;

	void initialize();
	void setupViewPort();
	void setupImGui();
	void createWindow(const char* windowName);
	void createProgram();
    void createRect();
	int createShader(GLenum type, const char* fname);
	const GLchar* readFile(const char* filename);

	void renderImGui();

	void finalize();
};
