#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>

#ifdef _DEBUG
#include "../../imgui/imgui.h"
#endif

#include "../../mainloop/MainLoop.h"
#include "../../render_engine/model/Mesh.h"
#include "../../utils/PhysicsUtils.h"

#include <iostream>


bool ViewPlane::checkIfInViewPlane(const RenderAABB& cookedBounds) const
{
	// Compute the projection interval radius of b onto L(t) = b.c + t * p.n
	// (This ACTUALLY works if you think about it... it's pretty cool yo)
	const float r =
		cookedBounds.extents.x * std::abs(normal.x) +
		cookedBounds.extents.y * std::abs(normal.y) +
		cookedBounds.extents.z * std::abs(normal.z);

	//std::cout << "\t\t\t" << -r << "\t<=\t" << getSignedDistance(cookedBounds.center) << std::endl;

	return -r <= getSignedDistance(cookedBounds.center);
}

float ViewPlane::getSignedDistance(const glm::vec3& testPoint) const
{
	//std::cout << "\t\t\t" << glm::dot(normal, testPoint - position) << std::endl;
	return glm::dot(normal, testPoint - position);
}

ViewFrustum ViewFrustum::createFrustumFromCamera(const Camera& camera)
{
	ViewFrustum frustum = ViewFrustum();
	const float halfVSide = camera.zFar * tanf(glm::radians(camera.fov) * .5f);
	const float halfHSide = halfVSide * camera.width / camera.height;
	const glm::vec3 frontMultFar = camera.zFar * camera.orientation;
	const glm::vec3 camRight = glm::normalize(glm::cross(camera.orientation, camera.up));
	const glm::vec3 camUp = glm::normalize(glm::cross(camRight, camera.orientation));

	// Create the frustum
	frustum.nearFace = { camera.orientation, camera.position + camera.zNear * camera.orientation };
	frustum.farFace = { -camera.orientation, camera.position + frontMultFar };
	frustum.rightFace = { glm::normalize(glm::cross(camUp, frontMultFar + camRight * halfHSide)), camera.position };
	frustum.leftFace = { glm::normalize(glm::cross(frontMultFar - camRight * halfHSide, camUp)), camera.position };
	frustum.topFace = { glm::normalize(glm::cross(frontMultFar + camUp * halfVSide, camRight)), camera.position };
	frustum.bottomFace = { glm::normalize(glm::cross(camRight, frontMultFar - camUp * halfVSide)), camera.position };

	return frustum;
}

bool ViewFrustum::checkIfInViewFrustum(const RenderAABB& bounds, const glm::mat4& modelMatrix) const
{
	RenderAABB cookedBounds = PhysicsUtils::fitAABB(bounds, modelMatrix);
	return topFace.checkIfInViewPlane(cookedBounds) &&
		bottomFace.checkIfInViewPlane(cookedBounds) &&
		rightFace.checkIfInViewPlane(cookedBounds) &&
		leftFace.checkIfInViewPlane(cookedBounds) &&
		farFace.checkIfInViewPlane(cookedBounds) &&
		nearFace.checkIfInViewPlane(cookedBounds);
}

Camera::Camera() : savedMouseX(-1.0), savedMouseY(-1.0)
{
	position = glm::vec3(0.0f, 0.0f, 0.0f);
	orientation = glm::vec3(0.0f, 0.0f, 1.0f);
	up = glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::mat4 Camera::calculateProjectionMatrix()
{
	// Note: When minimizing the window, there's a divide by zero error that happens with width/height
	if (width == 0.0f || height == 0.0f)
		return glm::mat4(1.0f);

	return glm::perspective(glm::radians(fov), (float)(width / height), zNear, zFar);
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

	glm::mat4 view = calculateViewMatrix();
	glm::mat4 projection = calculateProjectionMatrix();

	return glm::vec3(projection * view * glm::vec4(pointInSpace, 1.0f));
}

glm::vec3 Camera::clipSpacePositionToWordSpace(glm::vec3 clipSpacePosition)
{
	// Note: When minimizing the window, there's a divide by zero error that happens with width/height
	if (width == 0.0f || height == 0.0f)
		return glm::vec3(0.0f);

	glm::mat4 view = calculateViewMatrix();
	glm::mat4 projection = calculateProjectionMatrix();

	glm::vec4 result(glm::inverse(projection * view) * glm::vec4(clipSpacePosition, 1.0f));
	result.w = 1.0f / result.w;
	result *= result.w;
	return glm::vec3(result);
}

void Camera::addVirtualCamera(VirtualCamera* virtualCamera)
{
	// TODO: add checking for if the virtualcam was already in there
	virtualCameras.push_back(virtualCamera);
}

void Camera::removeVirtualCamera(VirtualCamera* virtualCamera)
{
	virtualCameras.erase(std::remove(virtualCameras.begin(), virtualCameras.end(), virtualCamera), virtualCameras.end());
}

void Camera::updateToVirtualCameras()
{
#ifdef _DEBUG
	if (!MainLoop::getInstance().playMode)
		return;
#endif

	if (!lockedCursor)
		return;

	//
	// Select the highest priority camera
	//
	VirtualCamera* currentVirtualCamera = nullptr;
	int currentHighestPriority = -2147483648;
	for (size_t i = 0; i < virtualCameras.size(); i++)
	{
		if (currentHighestPriority < virtualCameras[i]->priority)
		{
			currentHighestPriority = virtualCameras[i]->priority;
			currentVirtualCamera = virtualCameras[i];
		}
	}

	if (currentVirtualCamera == nullptr)
		return;

	//
	// Set myself to the highest priority virtual camera
	//
	position = currentVirtualCamera->position;
	orientation = currentVirtualCamera->orientation;
	up = currentVirtualCamera->up;
}

bool prevF11Keypressed;
void Camera::Inputs(GLFWwindow* window)			// NOTE: this event only gets called when not hovering over Imgui stuff
{
#ifdef _DEBUG
	if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS && prevF11Keypressed == GLFW_RELEASE)
	{
		//
		// Switch to game camera, free camera
		//
		MainLoop::getInstance().playMode = !MainLoop::getInstance().playMode;
	}
	prevF11Keypressed = glfwGetKey(window, GLFW_KEY_F11);
#endif

#ifdef _DEBUG
	// Don't do look around stuff unless if doing freecam mode
	if (MainLoop::getInstance().playMode)
#endif
	{
		if (!lockedCursor)
		{
			// Lock cursor if clicks
			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
			{
				lockedCursor = true;
#ifdef _DEBUG
				ImGui::SetMouseCursor(ImGuiMouseCursor_None);
#else
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
#endif
			}
		}
		else
		{
			// Unlock cursor if press F10		NOTE: Before this was F12, but F12 is reserved by Visual Studio to debug the application hahahahahahaha. WHY
			if (glfwGetKey(window, GLFW_KEY_F10) == GLFW_PRESS)
			{
				lockedCursor = false;
#ifdef _DEBUG
				ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
#else
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
#endif
			}
		}
		return;
	}
#ifdef _DEBUG
	else
		lockedCursor = false;
#endif

#ifdef _DEBUG
	//
	// Look around
	//
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		//
		// Move around in the scene (only when right button pressed)
		//
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			position += speed * orientation * MainLoop::getInstance().deltaTime;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			position += speed * -glm::cross(orientation, up) * MainLoop::getInstance().deltaTime;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			position += speed * -orientation * MainLoop::getInstance().deltaTime;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			position += speed * glm::cross(orientation, up) * MainLoop::getInstance().deltaTime;
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			position += speed * -up * MainLoop::getInstance().deltaTime;
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			position += speed * up * MainLoop::getInstance().deltaTime;

		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			speed = 50.0f;
		else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
			speed = 15.0f;

		//
		// Look around functionality
		//
		if (firstClicked)
		{
			firstClicked = false;
			glfwGetCursorPos(window, &savedMouseX, &savedMouseY);
		}

		//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		ImGui::SetMouseCursor(ImGuiMouseCursor_None);

		double mouseX, mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		float rotX = sensitivity * (float)(mouseY - savedMouseY) / height;
		float rotY = sensitivity * (float)(mouseX - savedMouseX) / height;
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
#endif
}
