#pragma once

#include "Animation.h"


struct AnimatedRope
{
	glm::mat4 globalTransformation;
	size_t boneId;
	glm::mat4 boneOffset;
	glm::mat4 parentTransform;
};


struct BlendTreeNode
{
	size_t animationIndex;
	float blendValue;						// NOTE: undefined behavior if the mixValue's aren't sorted asc
	std::string blendVariableReference;			// NOTE: only assign the first one, bc that's all that's used here
	float INTERNALcurrentNodeTime;
	float INTERNALnodeDuration;
};


class Animator
{
public:
	Animator() { }		// NOTE: Creation of the default constructor is just to appease the compiler
	Animator(std::vector<Animation>* animations, const std::vector<std::string>& boneTransformationsToKeepTrackOf = {});
	
	void updateAnimation(float deltaTime);
	void playAnimation(size_t animationIndex, float mixTime = 0.0f, bool looping = true);
	void playBlendTree(std::vector<BlendTreeNode> blendTreeNodes, float mixTime = 0.0f, bool looping = true);		// NOTE: force=true

	void setBlendTreeVariable(std::string variableName, float value);

	std::vector<glm::mat4>* getFinalBoneMatrices() { return &finalBoneMatrices; }

	inline float getCurrentTime() { return currentTime; }
	inline Animation* getCurrentAnimation() { return currentAnimation; }

	inline const AnimatedRope& getBoneTransformation(const std::string& boneName) { return boneTransformationsToKeepTrackOfMap[boneName]; }
	const void setBoneTransformation(const std::string& boneName, const glm::mat4& transformation);

	bool isAnimationFinished(size_t animationIndex, float deltaTime);		// NOTE: this doesn't work with blend trees (atm?)

	float animationSpeed = 1.0f;

private:
	std::vector<glm::mat4> finalBoneMatrices;

	bool currentUseBTN, nextUseBTN;
	Animation* currentAnimation, *nextAnimation;
	std::vector<BlendTreeNode> currentBTN, nextBTN;
	std::vector<Animation*> currentBTNAnims, nextBTNAnims;

	std::vector<Animation>* allAnimations;
	float currentTime, nextTime, mixTime, totalMixTime;

	bool loopingCurrent = true, loopingNext = true;

	int currentAnimationIndex = -1;
	int nextAnimationIndex = -1;

	// This contains two blend trees with normalized mixValues and a mixValue between the two animations
	std::map<std::string, float> blendTreeVariables;
	struct CalculateBoneTransformInput
	{
		AssimpNodeData* btCurrent0, * btCurrent1;
		Animation* btCurrent0Anim, * btCurrent1Anim;
		float btCurrent0NodeTime, btCurrent1NodeTime;
		float mixValueBTCurrent;

		AssimpNodeData* btNext0, * btNext1;
		Animation* btNext0Anim, * btNext1Anim;
		float btNext0NodeTime, btNext1NodeTime;
		float mixValueBTNext;

		float mixValueCurrentNext;
	};

	void calculateBoneTransform(CalculateBoneTransformInput input, const glm::mat4& globalRootInverseMatrix, const glm::mat4& parentTransform, std::map<std::string, BoneInfo>& boneInfoMap);
	void createNodeTransformCache(AssimpNodeData* node, Animation* animation, std::map<std::string, BoneInfo>& boneInfoMap);
	void invalidateCache(AssimpNodeData* node);

	//
	// For hair or other interactions
	//
	std::map<std::string, AnimatedRope> boneTransformationsToKeepTrackOfMap;
};

