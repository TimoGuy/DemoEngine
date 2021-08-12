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
	GLuint vert_shader, frag_shader, program_id;
	GLuint vbo, vao, ebo;

	void initialize();
	void setupViewPort();
	void createWindow(const char* windowName);
	void createProgram();
    void createRect();
	int createShader(GLenum type, const char* fname);
	const GLchar* readFile(const char* filename);
	void finalize();
};
