#pragma once


#include <assimp/scene.h>
#include <vector>
#include <string>
#include <map>

#include "Mesh.h"


class Animation;
struct ViewFrustum;
class Shader;
typedef unsigned int GLuint;

struct BoneInfo
{
	int id;
	glm::mat4 offset;
};

struct AnimationMetadata
{
	std::string animationName;
	bool trackXZRootMotion;
	float timestampSpeed = 1.0f;
};

class Model
{
public:
	Model();						// NOTE: Creation of the default constructor is just to appease the compiler
	Model(const char* path);
	Model(const char* path, std::vector<AnimationMetadata> animationMetadatas);
	Model(const std::vector<Vertex>& quadMesh);			// NOTE: this is for VoxelGroup class
	bool getIfInViewFrustum(const glm::mat4& modelMatrix, const ViewFrustum* viewFrustum, std::vector<bool>& out_whichMeshesInView);
	void render(const glm::mat4& modelMatrix, Shader* shaderOverride, const std::vector<bool>* whichMeshesInView, const std::vector<glm::mat4>* boneTransforms, RenderStage renderStage);

#ifdef _DEVELOP
	std::vector<std::string> getMaterialNameList();
	void TEMPrenderImguiModelBounds(glm::mat4 trans);
#endif

	auto& getBoneInfoMap() { return boneInfoMap; }
	int& getBoneCount() { return boneCounter; }
	auto& getAnimations() { return animations; }

	void setMaterials(std::map<std::string, Material*> materialMap);
	void setDepthPriorityOfMeshesWithMaterial(const std::string& materialName, float depthPriority);		// NOTE: This doesn't matter except for the transparent render pass

	std::vector<Mesh>& getMeshes() { return meshes; }

	glm::mat4 localTransform = glm::mat4(1.0f);

private:
	std::vector<Mesh> meshes;
	std::string directory;

	std::vector<Animation> animations;

	std::map<std::string, BoneInfo> boneInfoMap;
	int boneCounter = 0;

	const aiScene* scene;

	void loadModel(std::string path, std::vector<AnimationMetadata> animationMetadatas);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);

	void setVertexBoneDataToDefault(Vertex& vertex);
	void addVertexBoneData(Vertex& vertex, int boneId, float boneWeight);
	void extractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);
};

