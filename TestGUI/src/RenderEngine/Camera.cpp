#include "Camera.h"

#include <iostream>


void Camera::Matrix(float FOVdegrees, float zNear, float zFar, GLuint programID, const char* uniform)
{
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);
	
	view = glm::lookAt(position, position + orientation, up);
	projection = glm::perspective(glm::radians(FOVdegrees), (float)(width / height), zNear, zFar);

	glUniformMatrix4fv(glGetUniformLocation(programID, uniform), 1, GL_FALSE, glm::value_ptr(projection * view));
}

void Camera::Inputs(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		position += speed * orientation;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		position += speed * -glm::cross(orientation, up);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		position += speed * -orientation;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		position += speed * glm::cross(orientation, up);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		position += speed * -up;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		position += speed * up;

	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		speed = 0.4f;
	else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
		speed = 0.1f;

	//
	// Look around
	//
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		if (firstClicked)
		{
			firstClicked = false;
			glfwSetCursorPos(window, width / 2.0f, height / 2.0f);
		}

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

		double mouseX, mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		float rotX = sensitivity * (mouseY - (int)height / 2) / height;
		float rotY = sensitivity * (mouseX - (int)width / 2) / height;
		// NOTE: Below is for debug
		//std::cout << "Rotation X: " << rotX << "\tHeight/2: " << (height / 2.0) << "\tmouseY: " << mouseY << std::endl;

		glm::vec3 newOrientation = glm::rotate(orientation, glm::radians(-rotX), glm::normalize(glm::cross(orientation, up)));

		if (!(glm::angle(newOrientation, up) <= glm::radians(5.0f) || glm::angle(newOrientation, -up) <= glm::radians(5.0f)))
			orientation = newOrientation;

		orientation = glm::rotate(orientation, glm::radians(-rotY), up);

		glfwSetCursorPos(window, width / 2.0f, height / 2.0f);
	}
	else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		firstClicked = true;
	}
}
