#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>


struct RenderAABB;
class Camera;

struct ViewPlane
{
	glm::vec3 normal;
	glm::vec3 position;

	bool checkIfInViewPlane(const RenderAABB& cookedBounds);
	float getSignedDistance(const glm::vec3& point);
};

struct ViewFrustum
{
	ViewPlane topFace;
	ViewPlane bottomFace;

	ViewPlane rightFace;
	ViewPlane leftFace;

	ViewPlane farFace;
	ViewPlane nearFace;

	static ViewFrustum createFrustumFromCamera(const Camera& camera);
	bool checkIfInViewFrustum(const RenderAABB& bounds, const glm::mat4& modelMatrix);
};

struct VirtualCamera
{
	glm::vec3 position;
	glm::vec3 orientation;		// NOTE: this doesn't have to be normalized
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	int priority = 10;

	VirtualCamera() : position(glm::vec3(0.0f)), orientation(glm::vec3(0.0f, 0.0f, 1.0f)) {}
};

class Camera
{
public:
	glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 orientation = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	float fov = 45.0f;

	float width = 0;
	float height = 0;

	float zNear = 0.1f;
	float zFar = 2000.0f;

	float speed = 1.0f;
	float sensitivity = 100.0f;

	Camera();

	glm::mat4 calculateProjectionMatrix();
	glm::mat4 calculateViewMatrix();
	glm::vec3 PositionToClipSpace(glm::vec3 positionInSpace);
	glm::vec3 clipSpacePositionToWordSpace(glm::vec3 clipSpacePosition);

	void addVirtualCamera(VirtualCamera* virtualCamera);
	void removeVirtualCamera(VirtualCamera* virtualCamera);
	void updateToVirtualCameras();
	void Inputs(GLFWwindow* window);

	bool getLockedCursor() { return lockedCursor; }

private:
	bool lockedCursor = false;

	bool firstClicked = true;
	double savedMouseX, savedMouseY;

	std::vector<VirtualCamera*> virtualCameras;
};
