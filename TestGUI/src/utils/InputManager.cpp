#include "InputManager.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/gtx/norm.hpp>
#include "../mainloop/MainLoop.h"


void joystickCallback(int jid, int event);

InputManager::InputManager() : previousMouseX(-4206969), previousMouseY(-4206969), isJoystick1ConnectedAndIsGamepad(false)
{
	resetInputs();
	glfwSetJoystickCallback(joystickCallback);

	if (glfwJoystickPresent(GLFW_JOYSTICK_1) && glfwJoystickIsGamepad(GLFW_JOYSTICK_1))
	{
		isJoystick1ConnectedAndIsGamepad = true;
		std::cout << "JOYSTICK:: Joystick_1 is a gamepad and is already connected. Amai" << std::endl;
	}
}

InputManager& InputManager::getInstance()
{
	static InputManager instance;
	return instance;
}

void InputManager::resetInputs()
{
	leftStickX = 0;
	leftStickY = 0;
	rightStickX = 0;
	rightStickY = 0;

	prev_jumpPressed = jumpPressed;
	prev_attackWindupPressed = attackWindupPressed;
	prev_attackUnleashPressed = attackUnleashPressed;
	prev_interactPressed = interactPressed;
	prev_useItemPressed = useItemPressed;
	prev_transformPressed = transformPressed;
	prev_resetCamPressed = resetCamPressed;
	prev_inventoryPressed = inventoryPressed;
	prev_pausePressed = pausePressed;


	jumpPressed = false;
	attackWindupPressed = false;
	attackUnleashPressed = false;
	interactPressed = false;
	useItemPressed = false;
	transformPressed = false;
	resetCamPressed = false;
	inventoryPressed = false;
	pausePressed = false;
}

void joystickCallback(int jid, int event)
{
	if (jid != GLFW_JOYSTICK_1)		// NOTE: prevents other joysticks from joining!
		return;

	if (event == GLFW_CONNECTED)
	{
		const char* name = glfwGetJoystickName(GLFW_JOYSTICK_1);
		std::cout << "JOYSTICK:: Joystick \"" << name << "\" was connected!" << std::endl;

		if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1))
		{
			std::cout << "JOYSTICK:: Joystick_1 is a gamepad! NOW AVAILABLE FOR USE" << std::endl;
			InputManager::getInstance().INTERNALSetIJ1CAIG(true);	// Set is Joystick 1 connected and is gamepad
		}
	}
	else if (event == GLFW_DISCONNECTED)
	{
		std::cout << "JOYSTICK:: Joystick_1 was disconnected :(" << std::endl;

		InputManager::getInstance().INTERNALSetIJ1CAIG(false);
	}
}

bool isKeyPressed(GLFWwindow* window, int keyCode)
{
	return glfwGetKey(window, keyCode) == GLFW_PRESS;
}

bool isMouseButtonPressed(GLFWwindow* window, int mouseBtn)
{
	return glfwGetMouseButton(window, mouseBtn) == GLFW_PRESS;
}

void InputManager::INTERNALSetIJ1CAIG(bool flag)
{
	isJoystick1ConnectedAndIsGamepad = flag;
}

void InputManager::updateInputState()
{
	resetInputs();
	GLFWwindow* window = MainLoop::getInstance().window;

	//
	// GAMEPAD
	//
	if (isJoystick1ConnectedAndIsGamepad)
	{
		GLFWgamepadstate state;
		if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state) == GLFW_TRUE)
		{
			jumpPressed |= (bool)state.buttons[GLFW_GAMEPAD_BUTTON_A];
			attackWindupPressed |= (bool)(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.5f);  // (bool)state.buttons[GLFW_GAMEPAD_BUTTON_X];
			attackUnleashPressed |= (bool)state.buttons[GLFW_GAMEPAD_BUTTON_X];
			interactPressed |= (bool)state.buttons[GLFW_GAMEPAD_BUTTON_B];
			useItemPressed |= (bool)state.buttons[GLFW_GAMEPAD_BUTTON_Y];
			transformPressed |= (bool)state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER];
			resetCamPressed |= (bool)(state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > 0.5f);
			inventoryPressed |= (bool)state.buttons[GLFW_GAMEPAD_BUTTON_BACK];
			pausePressed |= (bool)state.buttons[GLFW_GAMEPAD_BUTTON_START];

			float _tempLeftStickX = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
			float _tempLeftStickY = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
			float _tempRightStickX = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
			float _tempRightStickY = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];

			const float deadzoneLeftStick = 0.3f;
			const float deadzoneLeftStickAxisAligned = 0.25f;
			const float deadzoneRightStickX = 0.2f;
			const float deadzoneRightStickY = 0.5f;
			const float rightStickSensitivity = 16.75f;

			if (glm::length2(glm::vec2(_tempLeftStickX, _tempLeftStickY)) > deadzoneLeftStick * deadzoneLeftStick)
			{
				leftStickX += glm::sign(_tempLeftStickX) * glm::max((abs(_tempLeftStickX) - deadzoneLeftStickAxisAligned) / (1.0f - deadzoneLeftStickAxisAligned), 0.0f);
				leftStickY -= glm::sign(_tempLeftStickY) * glm::max((abs(_tempLeftStickY) - deadzoneLeftStickAxisAligned) / (1.0f - deadzoneLeftStickAxisAligned), 0.0f);
			}

			if (abs(_tempRightStickX) > deadzoneRightStickX)
			{
				const float rightStickGamepadX = (_tempRightStickX + (_tempRightStickX < 0 ? deadzoneRightStickX : -deadzoneRightStickX)) / (1.0f - deadzoneRightStickX);
				rightStickX += glm::sign(rightStickGamepadX) * glm::pow(abs(rightStickGamepadX), 2.2f) * rightStickSensitivity * MainLoop::getInstance().deltaTime * 60.0f;
			}

			if (abs(_tempRightStickY) > deadzoneRightStickY)
			{
				const float rightStickGamepadY = (_tempRightStickY + (_tempRightStickY < 0 ? deadzoneRightStickY : -deadzoneRightStickY)) / (1.0f - deadzoneRightStickY);
				rightStickY += glm::sign(rightStickGamepadY) * glm::pow(abs(rightStickGamepadY), 1.7f) * rightStickSensitivity * MainLoop::getInstance().deltaTime * 60.0f;
			}

			//std::cout << "X: " << leftStickX << "\t\tY: " << leftStickY << std::endl;
		}
	}

	//
	// KEYBOPARD
	//
	if (true)
	{
		// KEYBOARD left stick
		leftStickX -= (float)isKeyPressed(window, GLFW_KEY_A);
		leftStickX += (float)isKeyPressed(window, GLFW_KEY_D);
		leftStickY += (float)isKeyPressed(window, GLFW_KEY_W);
		leftStickY -= (float)isKeyPressed(window, GLFW_KEY_S);

		// KEYBOARD right stick (mouse)
		double mouseX, mouseY;
		glfwGetCursorPos(MainLoop::getInstance().window, &mouseX, &mouseY);
		double deltaX = mouseX - previousMouseX;
		double deltaY = mouseY - previousMouseY;

		rightStickX += (float)deltaX;
		rightStickY += (float)deltaY;

		// KEYBOARD buttons
		jumpPressed |= isKeyPressed(window, GLFW_KEY_SPACE);
		attackWindupPressed |= isMouseButtonPressed(window, GLFW_MOUSE_BUTTON_RIGHT);
		attackUnleashPressed |= isMouseButtonPressed(window, GLFW_MOUSE_BUTTON_LEFT);
		interactPressed |= isKeyPressed(window, GLFW_KEY_E);
		useItemPressed |= isKeyPressed(window, GLFW_KEY_F);
		transformPressed |= isMouseButtonPressed(window, GLFW_MOUSE_BUTTON_RIGHT);
		resetCamPressed |= isKeyPressed(window, GLFW_KEY_LEFT_SHIFT);  // isMouseButtonPressed(window, GLFW_MOUSE_BUTTON_MIDDLE);
		inventoryPressed |= isKeyPressed(window, GLFW_KEY_TAB);
		pausePressed |= isKeyPressed(window, GLFW_KEY_ESCAPE);

		//
		// See if should lock cursor
		//
		if (MainLoop::getInstance().camera.getLockedCursor())
		{
			//
			// Lock the cursor
			// @Refactor: this code should not be here. It should probs be in mainloop ya think????
			//
			previousMouseX = (int)MainLoop::getInstance().camera.width / 2;		// NOTE: when setting cursor position as a double, the getCursorPos() function is slightly off, making the camera slowly move upwards
			previousMouseY = (int)MainLoop::getInstance().camera.height / 2;
			glfwSetCursorPos(MainLoop::getInstance().window, previousMouseX, previousMouseY);
		}
		else
		{
			// Regular update the previousMouse position
			previousMouseX = mouseX;
			previousMouseY = mouseY;
		}
	}

	// Update the on_ variables
	on_jumpPressed = (jumpPressed && !prev_jumpPressed);
	on_attackWindupPressed = (attackWindupPressed && !prev_attackWindupPressed);
	on_attackUnleashPressed = (attackUnleashPressed && !prev_attackUnleashPressed);
	on_interactPressed = (interactPressed && !prev_interactPressed);
	on_useItemPressed = (useItemPressed && !prev_useItemPressed);
	on_transformPressed = (transformPressed && !prev_transformPressed);
	on_resetCamPressed = (resetCamPressed && !prev_resetCamPressed);
	on_inventoryPressed = (inventoryPressed && !prev_inventoryPressed);
	on_pausePressed = (pausePressed && !prev_pausePressed);
}
