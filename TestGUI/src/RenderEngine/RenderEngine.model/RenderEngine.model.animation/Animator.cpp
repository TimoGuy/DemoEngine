#include "Animator.h"
#include <iostream>

Animator::Animator(Animation* animation) : deltaTime(0.0f)
{
	currentTime = 0.0f;
	currentAnimation = animation;

	finalBoneMatrices.reserve(100);

	for (unsigned int i = 0; i < 100; i++)
	{
		finalBoneMatrices.push_back(glm::mat4(1.0f));
	}
}


void Animator::updateAnimation(float deltaTime)
{
	Animator::deltaTime = deltaTime;
	if (currentAnimation)
	{
		currentTime += currentAnimation->getTicksPerSecond() * deltaTime;
		currentTime = fmod(currentTime, currentAnimation->getDuration());
		globalRootInverseMatrix = glm::inverse(currentAnimation->getRootNode().transformation);
		std::cout << "----------------" << std::endl;
		calculateBoneTransform(&currentAnimation->getRootNode(), glm::mat4(1.0f));
	}
}


void Animator::playAnimation(Animation* animation)
{
	currentAnimation = animation;
	currentTime = 0.0f;
}


void Animator::calculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)
{
	std::string nodeName = node->name;
	glm::mat4 nodeTransform = node->transformation;

	Bone* bone = currentAnimation->findBone(nodeName);

	if (bone)
	{
		bone->update(currentTime);
		nodeTransform = bone->getLocalTransform();
	}

	glm::mat4 globalTransformation = parentTransform * nodeTransform;

	auto boneInfoMap = currentAnimation->getBoneIdMap();
	if (boneInfoMap.find(nodeName) != boneInfoMap.end())
	{
		// Populate bone matrices for shader
		int index = boneInfoMap[nodeName].id;
		glm::mat4 offset = boneInfoMap[nodeName].offset;
		finalBoneMatrices[index] = globalRootInverseMatrix * globalTransformation * offset;
		std::cout << "BONE: " << index << std::endl;
	}

	// Recursively find childrens' bone transformation
	for (unsigned int i = 0; i < node->childrenCount; i++)
	{
		calculateBoneTransform(&node->children[i], globalTransformation);
	}
}
