#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class RenderableObject
{
public:
	RenderableObject(int id, float* vertices, float* normals);
	~RenderableObject();
	int getID();
	void render();

private:
	int id;
	GLuint vao = 0, vbo = 0;
	float* vertices;
	float* normals;
};

