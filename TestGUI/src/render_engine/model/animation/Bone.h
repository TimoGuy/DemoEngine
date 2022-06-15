#pragma once

#include <string>
#include <vector>
#include <glm/gtc/quaternion.hpp>


struct aiNodeAnim;

struct KeyPosition
{
	glm::vec3 position;
	float timeStamp;
};

struct KeyRotation
{
	glm::quat orientation;
	float timeStamp;
};

struct KeyScale
{
	glm::vec3 scale;
	float timeStamp;
};

class Bone
{
private:
	std::vector<KeyPosition>	positions;
	std::vector<KeyRotation>	rotations;
	std::vector<KeyScale>		scales;
	unsigned int numPositions, numRotations, numScales;

	std::string name;
	int id;

public:
	Bone() : id(-1) {}
	Bone(int id, const aiNodeAnim* channel);
	
	void update(float animationTime, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale);

	int getBoneId() { return id; }

	int getPositionIndex(float animationTime);
	int getRotationIndex(float animationTime);
	int getScaleIndex(float animationTime);

	void INTERNALmutateBoneAsRootBoneXZ();

private:
	float getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);
	glm::vec3 interpolatePosition(float animationTime);
	glm::quat interpolateRotation(float animationTime);
	glm::vec3 interpolateScaling(float animationTime);
};

