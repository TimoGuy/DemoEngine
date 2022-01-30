#include "Animator.h"

#include <glm/gtx/quaternion.hpp>
#include "../../../mainloop/MainLoop.h"

//// @Debug: for seeing how long matrix updates occur
//#include <chrono>
//#include <iostream>
//#include <iomanip>


Animator::Animator(std::vector<Animation>* animations, const std::vector<std::string>& boneTransformationsToKeepTrackOf) : allAnimations(animations), currentAnimation(nullptr), nextAnimation(nullptr)
{
	playAnimation(0, 0.0f, true, true);

	finalBoneMatrices.reserve(100);

	for (unsigned int i = 0; i < 100; i++)
	{
		finalBoneMatrices.push_back(glm::mat4(1.0f));
	}

	//
	// Register the bone transformations to keep track of
	//
	for (size_t i = 0; i < boneTransformationsToKeepTrackOf.size(); i++)
		boneTransformationsToKeepTrackOfMap.insert(std::pair<std::string, AnimatedRope>(boneTransformationsToKeepTrackOf[i], AnimatedRope()));
}


//long long accumTime = 0;
//size_t numCounts = 0;
void Animator::updateAnimation(float deltaTime)
{
	deltaTime *= animationSpeed;

#ifdef _DEVELOP
	// Don't run the animation update unless in playmode
	if (!MainLoop::getInstance().playMode)
		return;
#endif

	//auto start_time = std::chrono::high_resolution_clock::now();

	//
	// Update the current time of the current animation
	// @Copypasta... pretty much
	//
	if (currentUseBTN)
	{
		for (size_t i = 0; i < currentBTN.size(); i++)
		{
			float currentTimeBTN = currentBTN[i].INTERNALcurrentNodeTime;
			currentTimeBTN += currentBTNAnims[i]->getTicksPerSecond() * deltaTime;
			if (Animator::loopingCurrent)
				currentTimeBTN = fmod(currentTimeBTN, currentBTN[i].INTERNALnodeDuration);
			else
				currentTimeBTN = std::clamp(currentTimeBTN, 0.0f, currentBTN[i].INTERNALnodeDuration);

			currentBTN[i].INTERNALcurrentNodeTime = currentTimeBTN;
		}
	}
	else
	{
		currentTime += currentAnimation->getTicksPerSecond() * deltaTime;
		if (Animator::loopingCurrent)
			currentTime = fmod(currentTime, currentAnimation->getDuration());
		else
			currentTime = std::clamp(currentTime, 0.0f, currentAnimation->getDuration());
	}

	//
	// Use the nextanim??? (If there's mixtime yeah!)
	//
	if (mixTime > 0.0f)
	{
		//
		// Update the current time of the next animation
		// @Copypasta
		//
		if (nextUseBTN)
		{
			for (size_t i = 0; i < nextBTN.size(); i++)
			{
				float nextTimeBTN = nextBTN[i].INTERNALcurrentNodeTime;
				nextTimeBTN += nextBTNAnims[i]->getTicksPerSecond() * deltaTime;
				if (Animator::loopingNext)
					nextTimeBTN = fmod(nextTimeBTN, nextBTN[i].INTERNALnodeDuration);
				else
					nextTimeBTN = std::clamp(nextTimeBTN, 0.0f, nextBTN[i].INTERNALnodeDuration);

				nextBTN[i].INTERNALcurrentNodeTime = nextTimeBTN;
			}
		}
		else
		{
			nextTime += nextAnimation->getTicksPerSecond() * deltaTime;
			if (Animator::loopingNext)
				nextTime = fmod(nextTime, nextAnimation->getDuration());
			else
				nextTime = std::clamp(nextTime, 0.0f, nextAnimation->getDuration());
		}
	
		//
		// Deal with mixTime stuff
		//
		mixTime -= deltaTime;
		if (mixTime <= 0.0f)
		{
			// Switch over to primary animation only
			currentUseBTN = nextUseBTN;
			currentBTN = nextBTN;
			currentBTNAnims = nextBTNAnims;

			currentAnimationIndex = nextAnimationIndex;
			currentAnimation = nextAnimation;
			nextAnimationIndex = -1;
			currentTime = nextTime;
			loopingCurrent = loopingNext;
			nextTime = totalMixTime = -1.0f;
			nextAnimation = nullptr;

			//std::cout << "Invalidated cache" << std::endl;

			// Invalidate the cache for the bone bc the animation blend stopped (i.e. the mixed animation state ended).
			if (currentUseBTN)
				for (size_t i = 0; i < currentBTNAnims.size(); i++)
					invalidateCache(&currentBTNAnims[i]->getRootNode());
			else
				invalidateCache(&currentAnimation->getRootNode());
		}
	}

	//
	// Get out the important data once and then calculate the matrices!!!
	//
	const glm::mat4& globalRootInverseMatrix = currentUseBTN ? currentBTNAnims[0]->getGlobalRootInverseMatrix() : currentAnimation->getGlobalRootInverseMatrix();		// Doesn't matter which animation this comes from
	auto boneInfoMap = currentUseBTN ? currentBTNAnims[0]->getBoneIdMap() : currentAnimation->getBoneIdMap();															// Doesn't matter which animation this comes from

#ifdef _DEVELOP
	if (currentBTNAnims.size() > 2)
	{
		std::cout << "HEEEYYYYY ::NOTE:: You shouldve just made blendtrees support more than 2 anims!!!" << std::endl;
	}
#endif

	CalculateBoneTransformInput cbti;
	if (currentUseBTN)
	{
		cbti.btCurrent0 = &currentBTNAnims[0]->getRootNode();
		cbti.btCurrent0Anim = currentBTNAnims[0];
		cbti.btCurrent0NodeTime = currentBTN[0].INTERNALcurrentNodeTime;
		cbti.btCurrent1 = &currentBTNAnims[1]->getRootNode();
		cbti.btCurrent1Anim = currentBTNAnims[1];
		cbti.btCurrent1NodeTime = currentBTN[1].INTERNALcurrentNodeTime;
		cbti.mixValueBTCurrent = 0.5f;
	}
	else
	{
		cbti.btCurrent0 = &currentAnimation->getRootNode();
		cbti.btCurrent0Anim = currentAnimation;
		cbti.btCurrent0NodeTime = currentTime;
		cbti.mixValueBTCurrent = 0.0f;
	}

	if (mixTime > 0.0f)
	{
		if (nextUseBTN)
		{
			cbti.btNext0 = &nextBTNAnims[0]->getRootNode();
			cbti.btNext0Anim = nextBTNAnims[0];
			cbti.btNext0NodeTime = nextBTN[0].INTERNALcurrentNodeTime;
			cbti.btNext1 = &nextBTNAnims[1]->getRootNode();
			cbti.btNext1Anim = nextBTNAnims[1];
			cbti.btNext1NodeTime = nextBTN[1].INTERNALcurrentNodeTime;
			cbti.mixValueBTNext = 0.5f;
		}
		else
		{
			cbti.btNext0 = &nextAnimation->getRootNode();
			cbti.btNext0Anim = nextAnimation;
			cbti.btNext0NodeTime = nextTime;
			cbti.mixValueBTNext = 0.0f;
		}
		cbti.mixValueCurrentNext = 1.0f - mixTime / totalMixTime;
	}
	else
		cbti.mixValueCurrentNext = 0.0f;


	calculateBoneTransform(cbti, globalRootInverseMatrix, glm::mat4(1.0f), boneInfoMap);

	// @Optimize: Goal for skeletal animation is 0.01ms, however, right now it is taking 0.10ms in release mode, which is okay for now
	//auto end_time = std::chrono::high_resolution_clock::now();
	//auto time = end_time - start_time;
	//accumTime += time.count();
	//numCounts++;
	//
	//std::cout << std::left << std::setw(40) << "AVG TOTAL took " << accumTime / (double)numCounts / 1000000.0 << "ms to run.\n";			// DEBUG MODE AVG: 9.25ms :::: now: 7.15ms :::: now: 1.05ms :::: now: 0.74ms
}


void Animator::playAnimation(size_t animationIndex, float mixTime, bool looping, bool force)
{		// TODO: fix the forcing. It can be overridden somehow
	if (!force &&
		(currentAnimationIndex == (int)animationIndex ||
			nextAnimationIndex == (int)animationIndex))
		return;
	if (!force &&
		nextAnimation)
		return;		// NOTE: for now this is a blend and no-interrupt system, so when there's blending happening, there will be no other animation that can come in and blend as well

	assert(animationIndex < allAnimations->size());

	if (mixTime > 0.0f)
	{
		nextUseBTN = false;
		nextAnimationIndex = (int)animationIndex;

		Animator::loopingNext = looping;
		nextTime = 0.0f;
		nextAnimation = &(*allAnimations)[animationIndex];
	}
	else
	{
		currentUseBTN = false;
		currentAnimationIndex = (int)animationIndex;

		Animator::loopingCurrent = looping;
		currentTime = 0.0f;
		currentAnimation = &(*allAnimations)[animationIndex];
	}

	Animator::mixTime = Animator::totalMixTime = mixTime;

	// Invalidate the cache for the bone bc the animation changed.
	if (currentUseBTN)
		for (size_t i = 0; i < currentBTNAnims.size(); i++)
			invalidateCache(&currentBTNAnims[i]->getRootNode());
	else
		invalidateCache(&currentAnimation->getRootNode());
}


void Animator::playBlendTree(std::vector<BlendTreeNode> blendTreeNodes, float mixTime, bool looping)
{
	std::vector<Animation*> blendTreeAnims;
	for (size_t i = 0; i < blendTreeNodes.size(); i++)
	{
#ifdef _DEVELOP
		assert(blendTreeNodes[i].animationIndex < allAnimations->size());
#endif

		blendTreeAnims.push_back(&(*allAnimations)[blendTreeNodes[i].animationIndex]);

		blendTreeNodes[i].INTERNALcurrentNodeTime = 0.0f;
		blendTreeNodes[i].INTERNALnodeDuration = (*allAnimations)[blendTreeNodes[i].animationIndex].getDuration();
	}

	if (mixTime > 0.0f)
	{
		nextUseBTN = true;
		nextBTN = blendTreeNodes;
		nextBTNAnims = blendTreeAnims;

		Animator::loopingNext = looping;
		Animator::mixTime = Animator::totalMixTime = mixTime;
	}
	else
	{
		currentUseBTN = true;
		currentBTN = blendTreeNodes;
		currentBTNAnims = blendTreeAnims;

		Animator::loopingCurrent = looping;
	}

	Animator::mixTime = Animator::totalMixTime = mixTime;

	// Invalidate the cache for the bone bc the animation changed.
	if (currentUseBTN)
		for (size_t i = 0; i < currentBTNAnims.size(); i++)
			invalidateCache(&currentBTNAnims[i]->getRootNode());
	else
		invalidateCache(&currentAnimation->getRootNode());
}


void Animator::setBlendTreeVariable(std::string variableName, float value)
{

}


const void Animator::setBoneTransformation(const std::string& boneName, const glm::mat4& transformation)
{
	boneTransformationsToKeepTrackOfMap[boneName].globalTransformation = transformation;
	finalBoneMatrices[boneTransformationsToKeepTrackOfMap[boneName].boneId] =
		(currentUseBTN ? currentBTNAnims[0]->getGlobalRootInverseMatrix() : currentAnimation->getGlobalRootInverseMatrix()) *
		transformation *
		boneTransformationsToKeepTrackOfMap[boneName].boneOffset;
}


void Animator::calculateBoneTransform(CalculateBoneTransformInput input, const glm::mat4& globalRootInverseMatrix, const glm::mat4& parentTransform, std::map<std::string, BoneInfo>& boneInfoMap)
{
	glm::mat4 nodeTransform = input.btCurrent0->transformation;

	//
	// BLENDTREE JIGOKU
	//
	if (!input.btCurrent0->isCacheCreated)
		createNodeTransformCache(input.btCurrent0, input.btCurrent0Anim, boneInfoMap);
	if (input.btCurrent0->cacheBone)
	{
		// Calculate first blendtree
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;
		input.btCurrent0->cacheBone->update(input.btCurrent0NodeTime, position, rotation, scale);

		// Second part to this blendtree?
		if (input.mixValueBTCurrent > 0.0f)
		{
			// Calculate first blendtree, part 2!
			if (!input.btCurrent1->isCacheCreated)
				createNodeTransformCache(input.btCurrent1, input.btCurrent1Anim, boneInfoMap);
			if (input.btCurrent1->cacheBone)
			{
				glm::vec3 nextPosition;
				glm::quat nextRotation;
				glm::vec3 nextScale;
				input.btCurrent1->cacheBone->update(input.btCurrent1NodeTime, nextPosition, nextRotation, nextScale);

				position = glm::mix(position, nextPosition, input.mixValueBTCurrent);
				rotation = glm::slerp(rotation, nextRotation, input.mixValueBTCurrent);
				scale = glm::mix(scale, nextScale, input.mixValueBTCurrent);
			}
		}

		//
		// Second animation, if there is any!
		//
		if (input.mixValueCurrentNext > 0.0f)
		{
			// Calculate second blendtree
			if (!input.btNext0->isCacheCreated)
				createNodeTransformCache(input.btNext0, input.btNext0Anim, boneInfoMap);
			if (input.btNext0->cacheBone)
			{
				glm::vec3 position2;
				glm::quat rotation2;
				glm::vec3 scale2;
				input.btNext0->cacheBone->update(input.btNext0NodeTime, position2, rotation2, scale2);

				// Second part to this blendtree?
				if (input.mixValueBTNext > 0.0f)
				{
					// Calculate second blendtree, part 2!
					if (!input.btNext1->isCacheCreated)
						createNodeTransformCache(input.btNext1, input.btNext1Anim, boneInfoMap);
					if (input.btNext1->cacheBone)
					{
						glm::vec3 nextPosition;
						glm::quat nextRotation;
						glm::vec3 nextScale;
						input.btNext1->cacheBone->update(input.btNext1NodeTime, nextPosition, nextRotation, nextScale);

						position = glm::mix(position2, nextPosition, input.mixValueBTNext);
						rotation = glm::slerp(rotation2, nextRotation, input.mixValueBTNext);
						scale = glm::mix(scale2, nextScale, input.mixValueBTNext);
					}
				}

				// Final mixing
				position = glm::mix(position, position2, input.mixValueCurrentNext);
				rotation = glm::slerp(rotation, rotation2, input.mixValueCurrentNext);
				scale = glm::mix(scale, scale2, input.mixValueCurrentNext);
			}
		}

		// Convert this all to matrix4x4
		nodeTransform = glm::scale(glm::translate(glm::mat4(1.0f), position) * glm::toMat4(glm::normalize(rotation)), scale);
	}

	glm::mat4 globalTransformation = parentTransform * nodeTransform;

	//
	// Apply to the total transform!!!
	//
	AssimpNodeData* node = input.btCurrent0;
	if (node->cacheBoneInfoExists)
	{
		// Populate bone matrices for shader
		finalBoneMatrices[node->cacheBoneInfo_id] = globalRootInverseMatrix * globalTransformation * node->cacheBoneInfo_offset;

		// Insert the globalTransformation into a bone (if it's in boneTransformationsToKeepTrackOfMap)
		if (node->cacheAnimatedRope != nullptr)
		{
			node->cacheAnimatedRope->globalTransformation = globalTransformation;
			node->cacheAnimatedRope->boneId = node->cacheBoneInfo_id;
			node->cacheAnimatedRope->boneOffset = node->cacheBoneInfo_offset;
			node->cacheAnimatedRope->parentTransform = parentTransform;
		}
	}

	//
	// Recursively calculate childrens' bone transformation (@TODO: this part could be async I bet)
	//
	for (int i = 0; i < input.btCurrent0->childrenCount; i++)
	{
		CalculateBoneTransformInput nextInput = input;
		nextInput.btCurrent0 = &input.btCurrent0->children[i];
		if (input.mixValueBTCurrent)
			nextInput.btCurrent1 = &input.btCurrent1->children[i];
		if (input.mixValueCurrentNext)
		{
			nextInput.btNext0 = &input.btNext0->children[i];
			if (input.mixValueBTNext)
				nextInput.btNext1 = &input.btNext1->children[i];
		}

		calculateBoneTransform(nextInput, globalRootInverseMatrix, globalTransformation, boneInfoMap);
	}
}

void Animator::createNodeTransformCache(AssimpNodeData* node, Animation* animation, std::map<std::string, BoneInfo>& boneInfoMap)
{
	std::string nodeName = node->name;
	node->cacheBone = animation->findBone(nodeName);

	if (boneInfoMap.find(nodeName) != boneInfoMap.end())
	{
		node->cacheBoneInfo_id = boneInfoMap[nodeName].id;
		node->cacheBoneInfo_offset = boneInfoMap[nodeName].offset;
		node->cacheBoneInfoExists = true;

		//
		// Assign that to an ik thing
		//
		node->cacheAnimatedRope = nullptr;
		if (boneTransformationsToKeepTrackOfMap.find(nodeName) != boneTransformationsToKeepTrackOfMap.end())
		{
			node->cacheAnimatedRope = &boneTransformationsToKeepTrackOfMap[nodeName];
		}
	}
	else
	{
		node->cacheBoneInfoExists = false;
	}

	node->isCacheCreated = true;
}

bool Animator::isAnimationFinished(size_t animationIndex, float deltaTime)
{
	if (currentAnimationIndex != animationIndex)
		return false;

	float time = getCurrentTime() + getCurrentAnimation()->getTicksPerSecond() * deltaTime * animationSpeed;
	float duration = getCurrentAnimation()->getDuration();
	return time >= duration;
}

void Animator::invalidateCache(AssimpNodeData* node)
{
	node->isCacheCreated = false;
	for (size_t i = 0; i < node->childrenCount; i++)
		invalidateCache(&node->children[i]);
}
