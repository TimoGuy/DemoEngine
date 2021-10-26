#pragma once

#include "Bone.h"


struct AssimpNodeData
{
	glm::mat4 transformation;
	std::string name;
	int childrenCount;
	std::vector<AssimpNodeData> children;

	//
	// INTERNAL CACHE
	//
	bool isCacheCreated = false;
	Bone* cacheBone, *cacheNextBone;
	bool cacheNextBoneExists;
	int cacheBoneInfo_id;
	glm::mat4 cacheBoneInfo_offset;
	bool cacheBoneInfoExists;
};

class Animation
{
public:
	Animation() = default;
	Animation(const aiScene* scene, Model* model, std::string animationName);

	Bone* findBone(const std::string& name);

	inline float getTicksPerSecond() { return (float)ticksPerSecond; }
	inline float getDuration() { return duration; }
	inline AssimpNodeData& getRootNode() { return rootNode; }
	inline const std::map<std::string, BoneInfo>& getBoneIdMap() { return boneInfoMap; }
	inline const glm::mat4 getGlobalRootInverseMatrix() { return globalRootInverseMatrix; }

private:
	void readMissingBones(const aiAnimation* animation, Model& model);
	void readHierarchyData(AssimpNodeData& dest, const aiNode* src);

	float duration;
	int ticksPerSecond;
	std::map<std::string, Bone> bones;
	AssimpNodeData rootNode;
	glm::mat4 globalRootInverseMatrix;
	std::map<std::string, BoneInfo> boneInfoMap;
};

