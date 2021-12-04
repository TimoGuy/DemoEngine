#pragma once


#include <assimp/scene.h>
#include <vector>
#include <string>
#include <map>

#include "Mesh.h"


class Animation;
typedef unsigned int GLuint;

struct BoneInfo
{
	int id;
	glm::mat4 offset;
};

class Model
{
public:
	Model();						// NOTE: Creation of the default constructor is just to appease the compiler
	Model(const char* path);
	Model(const char* path, std::vector<std::string> animationNames);
	void render(const glm::mat4& modelMatrix, GLuint shaderIdOverride);

#ifdef _DEBUG
	void TEMPrenderImguiModelBounds(glm::mat4 trans);
#endif

	auto& getBoneInfoMap() { return boneInfoMap; }
	int& getBoneCount() { return boneCounter; }
	auto& getAnimations() { return animations; }

	void setMaterials(std::map<std::string, Material*> materialMap);

	const std::vector<Mesh>& getMeshes() { return meshes; }

	glm::mat4 localTransform = glm::mat4(1.0f);

private:
	std::vector<Texture> loadedTextures;
	std::vector<Mesh> meshes;
	std::string directory;

	std::vector<Animation> animations;

	std::map<std::string, BoneInfo> boneInfoMap;
	int boneCounter = 0;

	const aiScene* scene;

	void loadModel(std::string path, std::vector<std::string> animationNames);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);

	void setVertexBoneDataToDefault(Vertex& vertex);
	void addVertexBoneData(Vertex& vertex, int boneId, float boneWeight);
	void extractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);
};

