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
	Model() { scene = nullptr; }		// NOTE: Creation of the default constructor is just to appease the compiler
	Model(const char* path);
	Model(const char* path, std::vector<Animation>& animations, std::vector<int> animationIndices);
	void render(unsigned int shaderId);

	auto& getBoneInfoMap() { return boneInfoMap; }
	int& getBoneCount() { return boneCounter; }

private:
	std::vector<Texture> loadedTextures;
	std::vector<Mesh> meshes;
	std::string directory;

	std::map<std::string, BoneInfo> boneInfoMap;
	int boneCounter = 0;

	const aiScene* scene;

	void loadModel(std::string path, std::vector<Animation>& animations, std::vector<int> animationIndices);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* material, aiTextureType type, std::string typeName);

	void setVertexBoneDataToDefault(Vertex& vertex);
	void addVertexBoneData(Vertex& vertex, int boneId, float boneWeight);
	void extractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);
};

