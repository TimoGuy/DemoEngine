#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <map>
#include "../material/Material.h"

#define MAX_BONE_INFLUENCE 4

typedef unsigned int GLuint;

//
// This is an AABB struct used for strictly frustum culling.
//
struct RenderAABB
{
	glm::vec3 center;		// NOTE: multiply the baseObject->transform with this to get the world space
	glm::vec3 extents;		// NOTE: this is half the size of the aabb box
};

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
	int boneIds[MAX_BONE_INFLUENCE];
	float boneWeights[MAX_BONE_INFLUENCE];
	int numWeights = 0;
};

class Mesh
{
public:
	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, const RenderAABB& bounds, const std::string& materialName);
	~Mesh();

	void render(const glm::mat4& modelMatrix, GLuint shaderIdOverride, bool isTransparentQueue);

	void pickFromMaterialList(std::map<std::string, Material*> materialMap);

	const std::vector<Vertex>& getVertices() const { return vertices; }
	const std::vector<uint32_t>& getIndices() const { return indices; }

	RenderAABB bounds;

private:
	unsigned int VAO, VBO, EBO;
	void setupMesh();

	std::vector<Vertex>			vertices;
	std::vector<uint32_t>		indices;
	Material*					material;
	std::string					materialName;
};

