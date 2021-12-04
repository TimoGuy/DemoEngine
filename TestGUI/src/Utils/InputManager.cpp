#include "InputManager.h"

#include <GLFW/glfw3.h>
#include <iostream>
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

	jumpPressed = false;
	attackPressed = false;
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
			InputManager::getInstance().INTERNALSetIJ1CAIG(true);
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
			attackPressed |= (bool)state.buttons[GLFW_GAMEPAD_BUTTON_X];
			interactPressed |= (bool)state.buttons[GLFW_GAMEPAD_BUTTON_B];
			useItemPressed |= (bool)state.buttons[GLFW_GAMEPAD_BUTTON_Y];
			transformPressed |= (bool)state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER];
			resetCamPressed |= (bool)state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > 0.5f;
			inventoryPressed |= (bool)state.buttons[GLFW_GAMEPAD_BUTTON_BACK];
			pausePressed |= (bool)state.buttons[GLFW_GAMEPAD_BUTTON_START];

			float _tempLeftStickX = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
			float _tempLeftStickY = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
			float _tempRightStickX = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
			float _tempRightStickY = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];

			const float deadzoneLeftStick = 0.5f;
			const float deadzoneRightStickX = 0.3f;
			const float deadzoneRightStickY = 0.5f;
			const float rightStickSensitivity = 5.0f;

			if (abs(_tempLeftStickX) > deadzoneLeftStick || abs(_tempLeftStickY) > deadzoneLeftStick)
			{
				leftStickX += _tempLeftStickX;
				leftStickY += _tempLeftStickY * -1.0f;
			}
			if (abs(_tempRightStickX) > deadzoneRightStickX)
				rightStickX += (_tempRightStickX + (_tempRightStickX < 0 ? deadzoneRightStickX : -deadzoneRightStickX)) / (1.0f - deadzoneRightStickX) * rightStickSensitivity;
			if (abs(_tempRightStickY) > deadzoneRightStickY)
				rightStickY += (_tempRightStickY + (_tempRightStickY < 0 ? deadzoneRightStickY : -deadzoneRightStickY)) / (1.0f - deadzoneRightStickY) * rightStickSensitivity;
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

		rightStickX += deltaX;
		rightStickY += deltaY;

		// KEYBOARD buttons
		jumpPressed |= isKeyPressed(window, GLFW_KEY_SPACE);
		attackPressed |= isMouseButtonPressed(window, GLFW_MOUSE_BUTTON_LEFT);
		interactPressed |= isKeyPressed(window, GLFW_KEY_E);
		useItemPressed |= isKeyPressed(window, GLFW_KEY_F);
		transformPressed |= isMouseButtonPressed(window, GLFW_MOUSE_BUTTON_RIGHT);
		resetCamPressed |= isMouseButtonPressed(window, GLFW_MOUSE_BUTTON_MIDDLE);
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
}
