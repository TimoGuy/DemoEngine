#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

class RenderableObject
{
public:
	RenderableObject(int id, std::vector<float> vertices, std::vector<float> normals);
	~RenderableObject();
	int getID();
	void render();

private:
	int id;
	GLuint vao = 0, vbo = 0;
	std::vector<float> vertices;
	std::vector<float> normals;
};

