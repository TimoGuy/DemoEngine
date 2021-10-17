#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "../RenderEngine.material/Material.h"

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
	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, int materialIndex = -1);
	void render(unsigned int shaderId);

	void pickFromMaterialList(std::vector<Material*> materialList);

private:
	unsigned int VAO, VBO, EBO;
	void setupMesh();

	std::vector<Vertex>			vertices;
	std::vector<unsigned int>	indices;
	Material*					material;
	int							materialIndex;
};

