#pragma once

#include "Animation.h"


class Animator
{
public:
	Animator() { }		// NOTE: Creation of the default constructor is just to appease the compiler
	Animator(Animation* animation);
	
	void updateAnimation(float deltaTime);
	void playAnimation(Animation* animation);
	void calculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform);

	std::vector<glm::mat4> getFinalBoneMatrices() { return finalBoneMatrices; }

private:
	std::vector<glm::mat4> finalBoneMatrices;
	Animation* currentAnimation;
	float currentTime;
	float deltaTime;
};

