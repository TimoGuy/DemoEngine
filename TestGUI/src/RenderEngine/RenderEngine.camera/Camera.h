#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>


struct Bounds;
class Camera;

struct ViewPlane
{
	glm::vec3 normal;
	glm::vec3 position;

	bool checkIfInViewPlane(const Bounds& cookedBounds);
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
	bool checkIfInViewFrustum(const Bounds& bounds, const glm::mat4& modelMatrix);
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

	float speed = 0.1f;
	float sensitivity = 100.0f;

	Camera();

	glm::mat4 calculateProjectionMatrix();
	glm::mat4 calculateViewMatrix();
	glm::vec3 PositionToClipSpace(glm::vec3 positionInSpace);
	glm::vec3 clipSpacePositionToWordSpace(glm::vec3 clipSpacePosition);
	void Inputs(GLFWwindow* window);

private:
	bool firstClicked = true;
	double savedMouseX, savedMouseY;
};
