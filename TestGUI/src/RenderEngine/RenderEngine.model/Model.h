#pragma once


#include <assimp/scene.h>
#include <vector>
#include <string>
#include <map>

#include "Mesh.h"
class Animation;


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
	Model(const char* path, std::vector<int> animationIndices);
	void insertMeshesIntoSortedRenderQueue(std::map<GLuint, std::vector<Mesh*>>& sortedRenderQueue, const glm::mat4* modelMatrix, const std::vector<glm::mat4>* boneMatrices);
	void renderShadow(const glm::mat4* modelMatrix, const std::vector<glm::mat4>* boneMatrices);

	auto& getBoneInfoMap() { return boneInfoMap; }
	int& getBoneCount() { return boneCounter; }
	auto& getAnimations() { return animations; }

	void setMaterials(std::map<std::string, Material*> materialMap);

private:
	std::vector<Texture> loadedTextures;
	std::vector<Mesh> meshes;
	std::string directory;

	std::vector<Animation> animations;

	std::map<std::string, BoneInfo> boneInfoMap;
	int boneCounter = 0;

	const aiScene* scene;

	void loadModel(std::string path, std::vector<int> animationIndices);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* material, aiTextureType type, std::string typeName);

	void setVertexBoneDataToDefault(Vertex& vertex);
	void addVertexBoneData(Vertex& vertex, int boneId, float boneWeight);
	void extractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);
};

