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
	bool jumpPressed,		prev_jumpPressed,		on_jumpPressed,
		attackPressed,		prev_attackPressed,		on_attackPressed,
		interactPressed,	prev_interactPressed,	on_interactPressed,
		useItemPressed,		prev_useItemPressed,	on_useItemPressed,
		transformPressed,	prev_transformPressed,	on_transformPressed,
		resetCamPressed,	prev_resetCamPressed,	on_resetCamPressed,
		inventoryPressed,	prev_inventoryPressed,	on_inventoryPressed,
		pausePressed,		prev_pausePressed,		on_pausePressed;

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