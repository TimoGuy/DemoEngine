#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <fstream>
#include <string>

class RenderManager
{

public:
	RenderManager();
	~RenderManager();

private:

	GLFWwindow* window;
	GLuint vert_shader, frag_shader, program_id;
	GLuint vbo, vao;

	void initialize();
	void createWindow(const char* windowName);
	void createProgram();
    void createRect();
	int createShader(GLenum type, const char* fname);
	const GLchar* readFile(const char* filename);
	int run(void);
	void finalize();
};
