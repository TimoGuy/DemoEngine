#pragma once

#include "Animation.h"


class Animator
{
public:
	Animator() { }		// NOTE: Creation of the default constructor is just to appease the compiler
	Animator(std::vector<Animation>* animations);
	
	void updateAnimation(float deltaTime);
	void playAnimation(unsigned int animationIndex);
	void playAnimation(unsigned int animationIndex, float mixTime);
	void calculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform, bool useNextAnimation);

	std::vector<glm::mat4> getFinalBoneMatrices() { return finalBoneMatrices; }		// TODO: perhaps make this a reference

private:
	std::vector<glm::mat4> finalBoneMatrices;
	Animation* currentAnimation, *nextAnimation;
	std::vector<Animation>* animations;
	float currentTime, nextTime, mixTime, totalMixTime;
	float deltaTime;
};

