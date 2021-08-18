#pragma once

#include <assimp/scene.h>
#include <vector>
#include <string>
#include <map>

#include "Mesh.h"


struct BoneInfo
{
	int id;
	glm::mat4 offset;
};

class Model
{
public:
	Model(const char* path) { loadModel(path); }
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

	void loadModel(std::string path);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* material, aiTextureType type, std::string typeName);

	void setVertexBoneDataToDefault(Vertex& vertex);
	void addVertexBoneData(Vertex& vertex, int boneId, float boneWeight);
	void extractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);

	/*void BoneTransform(float time, std::vector<glm::mat4>& transformations)
	{
		glm::mat4 identity = glm::mat4(1.0f);

		float TicksPerSecond = (float)(scene->mAnimations[0]->mTicksPerSecond != 0 ? scene->mAnimations[0]->mTicksPerSecond : 25.0f);
		float TimeInTicks = time * TicksPerSecond;
		float AnimationTime = fmod(TimeInTicks, (float)scene->mAnimations[0]->mDuration);

		ReadNodeHierarchy(time, scene->mRootNode, identity);

		transformations.resize(boneCounter);

		for (unsigned int i = 0; i < boneCounter; i++)
		{
			transformations[i] = boneInfoMap.[i].
		}
	}*/
};

