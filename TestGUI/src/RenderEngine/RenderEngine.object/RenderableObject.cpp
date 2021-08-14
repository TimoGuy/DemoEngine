#include "RenderableObject.h"
#include <iostream>


RenderableObject::RenderableObject(int id, std::vector<float> vertices, std::vector<float> normals)
{
	this->id = id;
	this->vertices = vertices;
	this->normals = normals;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	std::cout << sizeof(RenderableObject::vertices) << std::endl;
	glBufferData(GL_ARRAY_BUFFER, RenderableObject::vertices.size() * sizeof(float), &RenderableObject::vertices.at(0), GL_STATIC_DRAW);

	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
}

RenderableObject::~RenderableObject()
{
}

int RenderableObject::getID()
{
	return this->id;
}

void RenderableObject::render()
{
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}