#pragma once

#include "Animation.h"


struct AnimatedRope
{
	glm::mat4 globalTransformation;
	size_t boneId;
	glm::mat4 boneOffset;
	glm::mat4 parentTransform;
};

class Animator
{
public:
	Animator() { }		// NOTE: Creation of the default constructor is just to appease the compiler
	Animator(std::vector<Animation>* animations, const std::vector<std::string>& boneTransformationsToKeepTrackOf = {});
	
	void updateAnimation(float deltaTime);
	void playAnimation(unsigned int animationIndex, float mixTime = 0.0f, bool looping = true, bool force = false);
	void calculateBoneTransform(AssimpNodeData* node, const glm::mat4& globalRootInverseMatrix, const glm::mat4& parentTransform, std::map<std::string, BoneInfo>& boneInfoMap, bool useNextAnimation);

	std::vector<glm::mat4>* getFinalBoneMatrices() { return &finalBoneMatrices; }

	inline float getCurrentTime() { return currentTime; }
	inline Animation* getCurrentAnimation() { return currentAnimation; }

	inline const AnimatedRope& getBoneTransformation(const std::string& boneName) { return boneTransformationsToKeepTrackOfMap[boneName]; }
	inline const void setBoneTransformation(const std::string& boneName, const glm::mat4& transformation) { finalBoneMatrices[boneTransformationsToKeepTrackOfMap[boneName].boneId] = currentAnimation->getGlobalRootInverseMatrix() * transformation * boneTransformationsToKeepTrackOfMap[boneName].boneOffset; }

private:
	std::vector<glm::mat4> finalBoneMatrices;
	Animation* currentAnimation, *nextAnimation;
	std::vector<Animation>* animations;
	float currentTime, nextTime, mixTime, totalMixTime;
	float deltaTime;

	bool loopingCurrent = true, loopingNext = true;

	unsigned int currentAnimationIndex = -1;		// This is nextAnimation's id when it's transitioning too btw.

	void invalidateCache(AssimpNodeData* node);

	//
	// For hair or other interactions
	//
	std::map<std::string, AnimatedRope> boneTransformationsToKeepTrackOfMap;
};

