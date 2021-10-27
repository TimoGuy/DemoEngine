#pragma once

#include "Animation.h"


class Animator
{
public:
	Animator() { }		// NOTE: Creation of the default constructor is just to appease the compiler
	Animator(std::vector<Animation>* animations);
	
	void updateAnimation(float deltaTime);
	void playAnimation(unsigned int animationIndex, float mixTime = 0.0f, bool looping = true, bool force = false);
	void calculateBoneTransform(AssimpNodeData* node, const glm::mat4& globalRootInverseMatrix, const glm::mat4& parentTransform, std::map<std::string, BoneInfo>& boneInfoMap, bool useNextAnimation);

	std::vector<glm::mat4>* getFinalBoneMatrices() { return &finalBoneMatrices; }

	inline float getCurrentTime() { return currentTime; }
	inline Animation* getCurrentAnimation() { return currentAnimation; }

private:
	std::vector<glm::mat4> finalBoneMatrices;
	Animation* currentAnimation, *nextAnimation;
	std::vector<Animation>* animations;
	float currentTime, nextTime, mixTime, totalMixTime;
	float deltaTime;

	bool loopingCurrent = true, loopingNext = true;

	unsigned int currentAnimationIndex = -1;		// This is nextAnimation's id when it's transitioning too btw.

	void invalidateCache(AssimpNodeData* node);
};

