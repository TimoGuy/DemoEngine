#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <map>
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
	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, const std::string& materialName);
	void render();

	void setupMatrixCache(const glm::mat4* modelMatrix, const std::vector<glm::mat4>* boneMatrices);
	void saveToSortedRenderQueue(std::map<GLuint, std::vector<Mesh*>>& sortedRenderQueue);

	void pickFromMaterialList(std::map<std::string, Material*> materialMap);

private:
	unsigned int VAO, VBO, EBO;
	void setupMesh();

	std::vector<Vertex>			vertices;
	std::vector<unsigned int>	indices;
	Material*					material;
	std::string					materialName;

	// Cache for renderObject stuff
	const glm::mat4*					savedModelMatrix;
	const std::vector<glm::mat4>*		savedBoneMatrices;		// TODO: don't forget about meeeee!!!
};

