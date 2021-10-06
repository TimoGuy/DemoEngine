#include "Animator.h"

#include <glm/gtx/quaternion.hpp>
#include "../../../MainLoop/MainLoop.h"

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


//long long accumTime = 0;
//size_t numCounts = 0;
void Animator::updateAnimation(float deltaTime)
{
	// Don't run the animation update unless in playmode
	if (!MainLoop::getInstance().playMode)
		return;

	//auto start_time = std::chrono::high_resolution_clock::now();

	Animator::deltaTime = deltaTime;
	if (currentAnimation)
	{
		currentTime += currentAnimation->getTicksPerSecond() * deltaTime;
		currentTime = fmod(currentTime, currentAnimation->getDuration());
		//std::cout << "----------------" << std::endl;
	}

	bool useNextAnimation = false; // @Optimize: This comment block makes no difference in exec time
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

			std::cout << "Invalidated cache" << std::endl;
			invalidateCache(&currentAnimation->getRootNode());		// Invalidate the cache for the bone bc the animation blend stopped (i.e. the mixed animation state ended).
		}
		else useNextAnimation = true;
	}

	//
	// Get out the important data once and then calculate the matrices!!!
	//
	const glm::mat4& globalRootInverseMatrix = currentAnimation->getGlobalRootInverseMatrix();
	auto boneInfoMap = currentAnimation->getBoneIdMap();
	AssimpNodeData& rootNode = currentAnimation->getRootNode();
	calculateBoneTransform(&rootNode, globalRootInverseMatrix, glm::mat4(1.0f), boneInfoMap, useNextAnimation);

	// @Optimize: Goal for skeletal animation is 0.01ms, however, right now it is taking 0.10ms in release mode, which is okay for now
	//auto end_time = std::chrono::high_resolution_clock::now();
	//auto time = end_time - start_time;
	//accumTime += time.count();
	//numCounts++;
	//
	//std::cout << std::left << std::setw(40) << "AVG TOTAL took " << accumTime / (double)numCounts / 1000000.0 << "ms to run.\n";			// DEBUG MODE AVG: 9.25ms :::: now: 7.15ms :::: now: 1.05ms :::: now: 0.74ms
}


void Animator::playAnimation(unsigned int animationIndex)
{
	if (currentAnimationIndex == animationIndex) return;
	if (nextAnimation) return;		// NOTE: for now this is a blend and no-interrupt system, so when there's blending happening, there will be no other animation that can come in and blend as well

	assert(animationIndex < animations->size());
	currentAnimationIndex = animationIndex;

	currentTime = 0.0f;
	currentAnimation = &(*animations)[animationIndex];
	invalidateCache(&currentAnimation->getRootNode());		// Invalidate the cache for the bone bc the animation changed.
}


void Animator::playAnimation(unsigned int animationIndex, float mixTime)
{
	if (currentAnimationIndex == animationIndex) return;
	if (nextAnimation) return;		// NOTE: for now this is a blend and no-interrupt system, so when there's blending happening, there will be no other animation that can come in and blend as well

	assert(animationIndex < animations->size());
	currentAnimationIndex = animationIndex;

	nextTime = 0.0f;
	nextAnimation = &(*animations)[animationIndex];
	Animator::mixTime = Animator::totalMixTime = mixTime;
	invalidateCache(&currentAnimation->getRootNode());		// Invalidate the cache for the bone bc the animation changed.
}


void Animator::calculateBoneTransform(AssimpNodeData* node, const glm::mat4& globalRootInverseMatrix, const glm::mat4& parentTransform, std::map<std::string, BoneInfo>& boneInfoMap, bool useNextAnimation)
{
	//
	// Create cache if not created
	//
	if (!node->isCacheCreated)
	{
		std::string nodeName = node->name;
		node->cacheBone = currentAnimation->findBone(nodeName);

		if (boneInfoMap.find(nodeName) != boneInfoMap.end())
		{
			node->cacheBoneInfo_id = boneInfoMap[nodeName].id;
			node->cacheBoneInfo_offset = boneInfoMap[nodeName].offset;
			node->cacheBoneInfoExists = true;
		}
		else
		{
			node->cacheBoneInfoExists = false;
		}

		if (useNextAnimation)
		{
			node->cacheNextBone = nextAnimation->findBone(nodeName);
			if (node->cacheNextBone)
				node->cacheNextBoneExists = true;
			else
				node->cacheNextBoneExists = false;
		}
		else
			node->cacheNextBoneExists = false;

		node->isCacheCreated = true;
	}
	glm::mat4 nodeTransform = node->transformation;

	//
	// Do matrix magic for this bone
	//
	if (node->cacheBone)
	{
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;

		node->cacheBone->update(currentTime, position, rotation, scale);																		// @Optimize: 0.003388ms to run on avg

		// 2nd Animation if exists
		if (useNextAnimation && node->cacheNextBoneExists)
		{
			glm::vec3 nextPosition;
			glm::quat nextRotation;
			glm::vec3 nextScale;
			node->cacheNextBone->update(nextTime, nextPosition, nextRotation, nextScale);
		
			float mixScaleFactor = 1.0f - mixTime / totalMixTime;
			position = glm::mix(position, nextPosition, mixScaleFactor);
			rotation = glm::slerp(rotation, nextRotation, mixScaleFactor);
			scale = glm::mix(scale, nextScale, mixScaleFactor);
		}

		// Convert this all to matrix4x4
		nodeTransform = glm::scale(glm::translate(glm::mat4(1.0f), position) * glm::toMat4(glm::normalize(rotation)), scale);		// @Optimize: Took 0.0043ms to run on avg
	}

	glm::mat4 globalTransformation = parentTransform * nodeTransform;

	if (node->cacheBoneInfoExists)
	{
		// Populate bone matrices for shader
		finalBoneMatrices[node->cacheBoneInfo_id] = globalRootInverseMatrix * globalTransformation * node->cacheBoneInfo_offset;
	}

	// Recursively find childrens' bone transformation
	for (int i = 0; i < node->childrenCount; i++)
	{
		calculateBoneTransform(&node->children[i], globalRootInverseMatrix, globalTransformation, boneInfoMap, useNextAnimation);
	}
}

void Animator::invalidateCache(AssimpNodeData* node)
{
	node->isCacheCreated = false;
	for (size_t i = 0; i < node->childrenCount; i++)
		invalidateCache(&node->children[i]);
}
