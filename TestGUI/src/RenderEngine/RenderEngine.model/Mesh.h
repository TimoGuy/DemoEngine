#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

#define MAX_BONE_INFLUENCE 4

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
	int boneIds[MAX_BONE_INFLUENCE];
	float boneWeights[MAX_BONE_INFLUENCE];
	int numWeights = 0;
};

struct Texture
{
	unsigned int id;
	std::string type;
	std::string path;
};

class Mesh
{
public:
	std::vector<Vertex>			vertices;
	std::vector<unsigned int>	indices;
	std::vector<Texture>		textures;

	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
	void render(unsigned int shaderId);

private:
	unsigned int VAO, VBO, EBO;
	void setupMesh();
};

