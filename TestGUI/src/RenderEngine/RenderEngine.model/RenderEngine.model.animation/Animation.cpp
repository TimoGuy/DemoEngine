#include "Animation.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <algorithm>


Animation::Animation(const std::string& animationPath, Model* model)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate | aiProcess_LimitBoneWeights);
	assert(scene && scene->mRootNode);

	auto animation = scene->mAnimations[0];		// #5 is idle for slime girl btw; #14 is the capoeira for slime_capoeira.fbx
	duration = animation->mDuration;
	ticksPerSecond = animation->mTicksPerSecond;

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
	readMissingBones(animation, *model);
}

Bone* Animation::findBone(const std::string& name)
{
	auto iterator = std::find_if(bones.begin(), bones.end(),
		[&](const Bone& bone)
		{
			return bone.getBoneName() == name;
		}
	);
	if (iterator == bones.end()) return nullptr;
	
	return &(*iterator);
}


//
// ---------- Private methods ------------
//
void Animation::readMissingBones(const aiAnimation* animation, Model& model)
{
	int size = animation->mNumChannels;

	auto& boneInfoMap = model.getBoneInfoMap();
	int& boneCount = model.getBoneCount();

	//
	// Read channels
	//
	for (unsigned int i = 0; i < size; i++)
	{
		auto channel = animation->mChannels[i];
		std::string boneName = channel->mNodeName.data;

		if (boneInfoMap.find(boneName) == boneInfoMap.end())
		{
			// Create new bone if unique
			boneInfoMap[boneName].id = boneCount;
			boneCount++;
		}

		bones.push_back(Bone(channel->mNodeName.data, boneInfoMap[channel->mNodeName.data].id, channel));
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
