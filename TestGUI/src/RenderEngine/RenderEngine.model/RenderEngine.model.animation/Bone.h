#pragma once

#include "../Model.h"
#include <glm/gtc/quaternion.hpp>


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
	int numPositions, numRotations, numScales;

	glm::mat4 localTransform;
	std::string name;
	int id;

public:
	Bone(const std::string& name, int id, const aiNodeAnim* channel);
	
	void update(float animationTime);
	
	glm::mat4 getLocalTransform() { return localTransform; }
	std::string getBoneName() const { return name; }
	int getBoneId() { return id; }

	int getPositionIndex(float animationTime);
	int getRotationIndex(float animationTime);
	int getScaleIndex(float animationTime);

private:
	float getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);
	glm::mat4 interpolatePosition(float animationTime);
	glm::mat4 interpolateRotation(float animationTime);
	glm::mat4 interpolateScaling(float animationTime);
};

