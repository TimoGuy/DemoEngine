#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "../RenderEngine.object/RenderableObject.h";

class RenderManager
{

public:
	RenderManager();
	~RenderManager();
	std::vector<RenderableObject>* getRenderList();
	int run(void);

private:

	GLFWwindow* window;
	GLuint program_id;
	std::vector<RenderableObject> renderList;

	void initialize();
	void createWindow(const char* windowName);
	void createProgram();
    void createRect();
	int createShader(GLenum type, const char* fname);
	const GLchar* readFile(const char* filename);
	void finalize();
};
