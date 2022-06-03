#pragma once

#include "Bone.h"


struct AnimatedRope;

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
	Bone* cacheBone;
	bool cacheBoneInfoExists;
	int cacheBoneInfo_id;
	glm::mat4 cacheBoneInfo_offset;
	AnimatedRope* cacheAnimatedRope;
};

class Animation
{
public:
	Animation() = default;
	Animation(const aiScene* scene, Model* model, AnimationMetadata animationMetadata);

	Bone* findBone(const std::string& name);

	inline std::string getName() { return name; }
	inline float getTicksPerSecond() { return (float)ticksPerSecond; }
	inline float getDuration() { return duration; }
	inline AssimpNodeData& getRootNode() { return rootNode; }
	inline const std::map<std::string, BoneInfo>& getBoneIdMap() { return boneInfoMap; }
	inline const glm::mat4 getGlobalRootInverseMatrix() { return globalRootInverseMatrix; }

private:
	void readMissingBones(const aiAnimation* animation, Model& model, AnimationMetadata animationMetadata);
	void readHierarchyData(AssimpNodeData& dest, const aiNode* src);

	std::string name;
	float duration;
	int ticksPerSecond;
	std::map<std::string, Bone> bones;
	AssimpNodeData rootNode;
	glm::mat4 globalRootInverseMatrix;
	std::map<std::string, BoneInfo> boneInfoMap;
};

