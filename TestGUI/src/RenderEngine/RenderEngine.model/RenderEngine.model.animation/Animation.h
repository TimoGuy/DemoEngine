#pragma once

#include "Bone.h"


struct AssimpNodeData
{
	glm::mat4 transformation;
	std::string name;
	int childrenCount;
	std::vector<AssimpNodeData> children;
};

class Animation
{
public:
	Animation() = default;
	Animation(const aiScene* scene, Model* model, unsigned int animationIndex);

	Bone* findBone(const std::string& name);

	inline float getTicksPerSecond() { return (float)ticksPerSecond; }
	inline float getDuration() { return duration; }
	inline const AssimpNodeData& getRootNode() { return rootNode; }
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

