#pragma once

#include <vector>
#include <string>

class InputManager
{
public:
	static InputManager& getInstance();

	void updateInputState();

	//
	// "Gamepad" interface (DON'T CHANGE PLS!!!!)
	//
	float leftStickX, leftStickY;
	float rightStickX, rightStickY;
	bool jumpPressed,
		attackPressed,
		interactPressed,
		useItemPressed,
		transformPressed,
		resetCamPressed,
		inventoryPressed,
		pausePressed;

	void INTERNALSetIJ1CAIG(bool flag);

private:
	InputManager();
	void resetInputs();
	
	double previousMouseX, previousMouseY;
	bool isJoystick1ConnectedAndIsGamepad;
};


///
///			=========================================
///			==============          =================
///			============    LEGEND    ===============
///			==============          =================
///			=========================================
/// 
///			WASD 		/		leftstick		- move
///			movemouse	/		rightstick		- adjust camera
///			space		/		A				- jump
///			leftclick	/		X				- attack
///			E			/		B				- interact
///			F			/		Y				- use item (usually drink water)
///			rightclick	/		RightBumper		- transform
///			middleclick /		lefttrigger		- reset camera
///			tab			/		backbtn			- open inventory(does not pause)
///			esc			/		start			- pause(only option is to exit game)
/// 