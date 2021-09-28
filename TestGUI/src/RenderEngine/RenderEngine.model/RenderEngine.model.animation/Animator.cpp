#include "Animator.h"

#include <glm/gtx/quaternion.hpp>


// @Debug: for seeing how long matrix updates occur
#include <chrono>
#include <iostream>
#include <iomanip>


Animator::Animator(std::vector<Animation>* animations) : deltaTime(0.0f), animations(animations), currentAnimation(nullptr), nextAnimation(nullptr)
{
	playAnimation(0);

	finalBoneMatrices.reserve(100);

	for (unsigned int i = 0; i < 100; i++)
	{
		finalBoneMatrices.push_back(glm::mat4(1.0f));
	}
}


void Animator::updateAnimation(float deltaTime)
{
	auto start_time = std::chrono::high_resolution_clock::now();

	Animator::deltaTime = deltaTime;
	if (currentAnimation)
	{
		currentTime += currentAnimation->getTicksPerSecond() * deltaTime;
		currentTime = fmod(currentTime, currentAnimation->getDuration());
		//std::cout << "----------------" << std::endl;
	}
	bool useNextAnimation = false;
	if (nextAnimation)
	{
		nextTime += nextAnimation->getTicksPerSecond() * deltaTime;
		nextTime = fmod(nextTime, nextAnimation->getDuration());
		mixTime -= deltaTime;

		if (mixTime <= 0.0f)
		{
			// Switch over to primary animation only
			currentTime = nextTime;
			currentAnimation = nextAnimation;
			nextTime = totalMixTime = -1.0f;
			nextAnimation = nullptr;
		}
		else useNextAnimation = true;
	}
	calculateBoneTransform(&currentAnimation->getRootNode(), glm::mat4(1.0f), useNextAnimation);

	auto end_time = std::chrono::high_resolution_clock::now();
	auto time = end_time - start_time;
	
	std::cout << std::left << std::setw(40) << "TOTAL took " << time.count() / 1000000.0 << "ms to run.\n";
}


void Animator::playAnimation(unsigned int animationIndex)
{
	if (nextAnimation) return;		// NOTE: for now this is a blend and no-interrupt system, so when there's blending happening, there will be no other animation that can come in and blend as well

	assert(animationIndex < animations->size());
	currentTime = 0.0f;
	currentAnimation = &(*animations)[animationIndex];
}


void Animator::playAnimation(unsigned int animationIndex, float mixTime)
{
	if (nextAnimation) return;		// NOTE: for now this is a blend and no-interrupt system, so when there's blending happening, there will be no other animation that can come in and blend as well

	assert(animationIndex < animations->size());
	nextTime = 0.0f;
	nextAnimation = &(*animations)[animationIndex];
	Animator::mixTime = Animator::totalMixTime = mixTime;
}


void Animator::calculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform, bool useNextAnimation)
{

	//std::cout << "Bone: " << node->name << std::endl;
	std::string nodeName = node->name;
	glm::mat4 nodeTransform = node->transformation;

	Bone* bone = currentAnimation->findBone(nodeName);
	if (bone)
	{
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;

		//auto start_time = std::chrono::high_resolution_clock::now();
		bone->update(currentTime, position, rotation, scale);

		if (useNextAnimation)
		{
			Bone* nextBone = nextAnimation->findBone(nodeName);
			if (nextBone)
			{
				glm::vec3 nextPosition;
				glm::quat nextRotation;
				glm::vec3 nextScale;
				nextBone->update(nextTime, nextPosition, nextRotation, nextScale);

				float mixScaleFactor = 1.0 - mixTime / totalMixTime;
				position = glm::mix(position, nextPosition, mixScaleFactor);
				rotation = glm::slerp(rotation, nextRotation, mixScaleFactor);
				scale = glm::mix(scale, nextScale, mixScaleFactor);
			}
		}

		// Convert this all to matrix4x4
		nodeTransform = glm::scale(glm::translate(glm::mat4(1.0f), position) * glm::toMat4(glm::normalize(rotation)), scale);

		//auto end_time = std::chrono::high_resolution_clock::now();
		//auto time = end_time - start_time;
		//
		//std::cout << std::left << std::setw(40) << ("updateMatrix(" + nodeName + ") took ") << time.count() / 1000000.0 << "ms to run.\n";
	}

	glm::mat4 globalTransformation = parentTransform * nodeTransform;

	auto boneInfoMap = currentAnimation->getBoneIdMap();
	if (boneInfoMap.find(nodeName) != boneInfoMap.end())
	{
		// Populate bone matrices for shader
		int index = boneInfoMap[nodeName].id;
		glm::mat4 offset = boneInfoMap[nodeName].offset;
		finalBoneMatrices[index] = currentAnimation->getGlobalRootInverseMatrix() * globalTransformation * offset;
		//finalBoneMatrices[index] = globalTransformation;
		//std::cout << "BONE: " << index << std::endl;
	}

	// Recursively find childrens' bone transformation
	for (unsigned int i = 0; i < node->childrenCount; i++)
	{
		calculateBoneTransform(&node->children[i], globalTransformation, useNextAnimation);
	}
}
