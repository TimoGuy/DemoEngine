#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

class Camera
{
public:
	glm::vec3 position = glm::vec3(0.0f, 0.0f, 2.0f);
	glm::vec3 orientation = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	float width = 0;
	float height = 0;

	float speed = 0.1f;
	float sensitivity = 100.0f;

	Camera() { }

	void Matrix(float FOVdegrees, float zNear, float zFar, GLuint programID, const char* uniform);
	void Inputs(GLFWwindow* window);

private:
	bool firstClicked = true;
};

