#include "RenderableObject.h"

RenderableObject::RenderableObject(int id, float* vertices, float* normals)
{
	this->id = id;
	this->vertices = vertices;
	this->normals = normals;
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
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glDrawArrays(GL_POINTS, 0, 6);
}