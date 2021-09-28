#include "Bone.h"


Bone::Bone(int id, const aiNodeAnim* channel) : id(id)
{
	numPositions = channel->mNumPositionKeys;
	for (unsigned int i = 0; i < numPositions; i++)
	{
		aiVector3D aiPosition = channel->mPositionKeys[i].mValue;
		float timeStamp = channel->mPositionKeys[i].mTime;
		KeyPosition data;
		data.position = glm::vec3(aiPosition.x, aiPosition.y, aiPosition.z);				// FIXME: For some reason the positioning of the models isn't quite right. It's like the bones are twice as long as they're supposed to be... like the bone moves twice in bone space...
		data.timeStamp = timeStamp;
		positions.push_back(data);
	}

	numRotations = channel->mNumRotationKeys;
	for (unsigned int i = 0; i < numRotations; i++)
	{
		aiQuaternion aiOrientation = channel->mRotationKeys[i].mValue;
		float timeStamp = channel->mRotationKeys[i].mTime;
		KeyRotation data;
		data.orientation = glm::quat(aiOrientation.w, aiOrientation.x, aiOrientation.y, aiOrientation.z);
		data.timeStamp = timeStamp;
		rotations.push_back(data);
	}

	numScales = channel->mNumScalingKeys;
	for (unsigned int i = 0; i < numScales; i++)
	{
		aiVector3D scale = channel->mScalingKeys[i].mValue;
		float timeStamp = channel->mScalingKeys[i].mTime;
		KeyScale data;
		data.scale = glm::vec3(scale.x, scale.y, scale.z);
		data.timeStamp = timeStamp;
		scales.push_back(data);
	}
}


void Bone::update(float animationTime, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale)
{
	translation =	interpolatePosition(animationTime);
	rotation =		interpolateRotation(animationTime);
	scale =			interpolateScaling(animationTime);
}


int Bone::getPositionIndex(float animationTime)
{
	for (unsigned int i = 0; i < numPositions - 1; i++)
	{
		if (animationTime < positions[i + 1].timeStamp)
			return i;
	}
	assert(0);
}


int Bone::getRotationIndex(float animationTime)
{
	for (unsigned int i = 0; i < numRotations - 1; i++)
	{
		if (animationTime < rotations[i + 1].timeStamp)
			return i;
	}
	assert(0);
}


int Bone::getScaleIndex(float animationTime)
{
	for (unsigned int i = 0; i < numScales - 1; i++)
	{
		if (animationTime < scales[i + 1].timeStamp)
			return i;
	}
	assert(0);
}


//
// -------------------- Private functions --------------------
//
float Bone::getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
{
	float scaleFactor = 0.0f;
	float midWayLength = animationTime - lastTimeStamp;
	float framesDiff = nextTimeStamp - lastTimeStamp;
	scaleFactor = midWayLength / framesDiff;
	return scaleFactor;
}


glm::vec3 Bone::interpolatePosition(float animationTime)
{
	if (1 == numPositions)
		//return glm::translate(glm::mat4(1.0f), positions[0].position);
		return positions[0].position;

	int p0Index = getPositionIndex(animationTime);
	int p1Index = p0Index + 1;
	float scaleFactor = getScaleFactor(positions[p0Index].timeStamp, positions[p1Index].timeStamp, animationTime);
	glm::vec3 finalPosition = glm::mix(positions[p0Index].position, positions[p1Index].position, scaleFactor);
	//return glm::translate(glm::mat4(1.0f), finalPosition);
	return finalPosition;
}


glm::quat Bone::interpolateRotation(float animationTime)
{
	if (1 == numRotations)
	{
		auto rotation = glm::normalize(rotations[0].orientation);
		//return glm::toMat4(rotation);
		return rotation;
	}

	int p0Index = getRotationIndex(animationTime);
	int p1Index = p0Index + 1;
	float scaleFactor = getScaleFactor(rotations[p0Index].timeStamp, rotations[p1Index].timeStamp, animationTime);
	glm::quat finalRotation = glm::slerp(rotations[p0Index].orientation, rotations[p1Index].orientation, scaleFactor);
	//return glm::toMat4(glm::normalize(finalRotation));
	return finalRotation;
}


glm::vec3 Bone::interpolateScaling(float animationTime)
{
	if (1 == numScales)
		//return glm::scale(glm::mat4(1.0f), scales[0].scale);
		return scales[0].scale;

	int p0Index = getScaleIndex(animationTime);
	int p1Index = p0Index + 1;
	float scaleFactor = getScaleFactor(scales[p0Index].timeStamp, scales[p1Index].timeStamp, animationTime);
	glm::vec3 finalScale = glm::mix(scales[p0Index].scale, scales[p1Index].scale, scaleFactor);
	//return glm::scale(glm::mat4(1.0f), finalScale);
	return finalScale;
}
