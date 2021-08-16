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
	Animation(const std::string& animationPath, Model* model);

	Bone* findBone(const std::string& name);

	inline float getTicksPerSecond() { return ticksPerSecond; }
	inline float getDuration() { return duration; }
	inline const AssimpNodeData& getRootNode() { return rootNode; }
	inline const std::map<std::string, BoneInfo>& getBoneIdMap() { return boneInfoMap; }

private:
	void readMissingBones(const aiAnimation* animation, Model& model);
	void readHierarchyData(AssimpNodeData& dest, const aiNode* src);

	float duration;
	int ticksPerSecond;
	std::vector<Bone> bones;
	AssimpNodeData rootNode;
	std::map<std::string, BoneInfo> boneInfoMap;
};

