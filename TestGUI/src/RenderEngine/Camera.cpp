#include "Camera.h"

#include "../ImGui/imgui.h"

#include <iostream>


Camera::Camera() : savedMouseX(-1.0), savedMouseY(-1.0)
{
	position = glm::vec3(0.0f, 0.0f, 2.0f);
	orientation = glm::vec3(0.0f, 0.0f, -1.0f);
	up = glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::mat4 Camera::calculateProjectionMatrix()
{
	// Note: When minimizing the window, there's a divide by zero error that happens with width/height
	if (width == 0.0f || height == 0.0f)
		return glm::mat4(1.0f);

	return glm::perspective(glm::radians(45.0f), (float)(width / height), 0.1f, 2000.0f);		// NOTE: Hardcoded!!!
}

glm::mat4 Camera::calculateViewMatrix()
{
	return glm::lookAt(position, position + orientation, up);
}

glm::vec3 Camera::PositionToClipSpace(glm::vec3 pointInSpace)
{
	// Note: When minimizing the window, there's a divide by zero error that happens with width/height
	if (width == 0.0f || height == 0.0f)
		return glm::vec3(0.0f);

	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);

	view = glm::lookAt(position, position + orientation, up);
	projection = glm::perspective(glm::radians(45.0f), (float)(width / height), 0.1f, 2000.0f);		// NOTE: Hardcoded!!!!

	return glm::vec3(projection * view * glm::vec4(pointInSpace, 1.0f));
}

void Camera::Inputs(GLFWwindow* window)
{
	//std::cout << "Pos:\tx:\t" << position.x << "\ty:\t" << position.y << "\tz:\t" << position.z << std::endl;
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
			glfwGetCursorPos(window, &savedMouseX, &savedMouseY);
		}

		//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		ImGui::SetMouseCursor(ImGuiMouseCursor_None);

		double mouseX, mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		float rotX = sensitivity * (mouseY - savedMouseY) / height;
		float rotY = sensitivity * (mouseX - savedMouseX) / height;
		// NOTE: Below is for debug
		//std::cout << "Rotation X: " << rotX << "\tHeight/2: " << (height / 2.0) << "\tmouseY: " << mouseY << std::endl;

		glm::vec3 newOrientation = glm::rotate(orientation, glm::radians(-rotX), glm::normalize(glm::cross(orientation, up)));

		if (!(glm::angle(newOrientation, up) <= glm::radians(5.0f) || glm::angle(newOrientation, -up) <= glm::radians(5.0f)))
			orientation = newOrientation;

		orientation = glm::rotate(orientation, glm::radians(-rotY), up);

		glfwSetCursorPos(window, savedMouseX, savedMouseY);
	}
	else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
	{
		//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
		if (!firstClicked) glfwSetCursorPos(window, width / 2, height / 2);
		firstClicked = true;
	}
}
