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
	bool jumpPressed,			prev_jumpPressed,			on_jumpPressed,
		attackWindupPressed,	prev_attackWindupPressed,	on_attackWindupPressed,
		attackUnleashPressed,	prev_attackUnleashPressed,	on_attackUnleashPressed,
		interactPressed,		prev_interactPressed,		on_interactPressed,
		useItemPressed,			prev_useItemPressed,		on_useItemPressed,
		transformPressed,		prev_transformPressed,		on_transformPressed,
		resetCamPressed,		prev_resetCamPressed,		on_resetCamPressed,
		inventoryPressed,		prev_inventoryPressed,		on_inventoryPressed,
		pausePressed,			prev_pausePressed,			on_pausePressed;

	void INTERNALSetIJ1CAIG(bool flag);

private:
	InputManager();
	void resetInputs();
	
	double previousMouseX, previousMouseY;
	bool isJoystick1ConnectedAndIsGamepad;
};


///
///		=========================================
///		==============          =================
///		============   CONTROLS   ===============
///		==============          =================
///		=========================================
///
///		NOTE: Bumpers and Triggers are lumped together (i.e. Left Trigger == Left Bumper)
///
///		esc						/		start						- pause(only option is to exit game)
///		tab						/		backbtn						- open inventory(does not pause)
///		space					/		A							- jump
///		E						/		B							- interact
///		F						/		Y							- use item (NOTE: currently unused)
///		WASD 					/		leftstick					- move
///		movemouse				/		rightstick					- adjust camera
///		leftshift 				/		lefttrigger					- focus (reset camera / target enemy)
///
///		**WEAPON SHEATHED**
///		leftshift & rightclick	/ 		lefttrigger & righttrigger	- will/brandish weapon out
///		leftclick				/		X							- run
///		rightclick				/		righttrigger				- bring up transformation menu (and then you can transform)
///
///		**WEAPON BRANDISHED**
///		leftshift & rightclick	/ 		lefttrigger & righttrigger	- will/sheath weapon back away
///		leftclick				/		X							- attack (press while holding righttrigger to unleash heavy attack)
///		rightclick				/		righttrigger				- charge heavy attack
/// 