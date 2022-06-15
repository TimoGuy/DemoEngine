#include "Animation.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <algorithm>
#include "../Model.h"


Animation::Animation(const aiScene* scene, Model* model, AnimationMetadata animationMetadata)
{
	assert(scene && scene->mRootNode);

	aiAnimation* animation = nullptr;
	for (size_t i = 0; i < scene->mNumAnimations; i++)
	{
		if (scene->mAnimations[i]->mName.C_Str() == animationMetadata.animationName)
		{
			animation = scene->mAnimations[i];
			break;
		}
	}

	// Make sure that the animation exists
	assert(animation != nullptr);

	name = animation->mName.C_Str();
	duration = (float)animation->mDuration;
	ticksPerSecond = (int)animation->mTicksPerSecond * animationMetadata.timestampSpeed;

	aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
	globalTransformation = globalTransformation.Inverse();
	{
		glm::mat4 to = glm::mat4(1.0f);
		aiMatrix4x4 from = globalTransformation;
		//the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
		to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
		to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
		to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
		to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
		globalRootInverseMatrix = to;
	}

	readHierarchyData(rootNode, scene->mRootNode);
	readMissingBones(animation, *model, animationMetadata);
}

Bone* Animation::findBone(const std::string& name)
{
	if (bones.find(name) != bones.end())
	{
		return &bones[name];
	}

	return nullptr;
}


//
// ---------- Private methods ------------
//
void Animation::readMissingBones(const aiAnimation* animation, Model& model, AnimationMetadata animationMetadata)
{
	uint32_t size = animation->mNumChannels;

	auto& boneInfoMap = model.getBoneInfoMap();
	int& boneCount = model.getBoneCount();

	//
	// Read channels
	//
	for (uint32_t i = 0; i < size; i++)
	{
		auto channel = animation->mChannels[i];
		std::string boneName = channel->mNodeName.data;

		if (boneInfoMap.find(boneName) == boneInfoMap.end())
		{
			// Create new bone if unique
			boneInfoMap[boneName].id = boneCount;
			boneCount++;
		}

		Bone newBone(boneInfoMap[boneName].id, channel);
		if (animationMetadata.trackXZRootMotion && boneName == "Root")
			newBone.INTERNALmutateBoneAsRootBoneXZ();

		bones.insert(
			bones.begin(),
			std::pair<std::string, Bone>(
				channel->mNodeName.data,
				newBone
			)
		);
	}
	Animation::boneInfoMap = boneInfoMap;
}

void Animation::readHierarchyData(AssimpNodeData& dest, const aiNode* src)
{
	assert(src);

	dest.name = src->mName.data;
	{
		glm::mat4 to = glm::mat4(1.0f);
		aiMatrix4x4 from = src->mTransformation;
		//the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
		to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
		to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
		to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
		to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
		dest.transformation = to;
	}
	dest.childrenCount = src->mNumChildren;

	// Recursively read hierarchy data
	for (unsigned int i = 0; i < src->mNumChildren; i++)
	{
		AssimpNodeData newData;
		readHierarchyData(newData, src->mChildren[i]);
		dest.children.push_back(newData);
	}
}
