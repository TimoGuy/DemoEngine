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

	bool pausePressed,				prev_pausePressed,				on_pausePressed,
		inventoryPressed,			prev_inventoryPressed,			on_inventoryPressed,

		jumpPressed,				prev_jumpPressed,				on_jumpPressed,
		interactPressed,			prev_interactPressed,			on_interactPressed,
		useItemPressed,				prev_useItemPressed,			on_useItemPressed,
		focusPressed,				prev_focusPressed,				on_focusPressed,

		willWeaponPressed,			prev_willWeaponPressed,			on_willWeaponPressed,
		dashPressed,				prev_dashPressed,				on_dashPressed,
		transformPressed,			prev_transformPressed,			on_transformPressed,

		attackPressed,				prev_attackPressed,				on_attackPressed,
		chargeHeavyAttackPressed,	prev_chargeHeavyAttackPressed,	on_chargeHeavyAttackPressed;

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
///		WASD 					/		leftstick					- move
///		movemouse				/		rightstick					- adjust camera
///
///		esc						/		start						- pause(only option is to exit game)
///		tab						/		backbtn						- open inventory(does not pause)
///
///		space					/		A							- jump
///		E						/		B							- interact
///		F						/		Y							- use item (NOTE: currently unused)
///		leftshift 				/		lefttrigger					- focus (reset camera / target enemy)
///
///		**WEAPON SHEATHED**
///		leftshift & rightclick	/ 		lefttrigger & righttrigger	- will/brandish weapon out
///		leftclick				/		X							- dash
///		rightclick				/		righttrigger				- bring up transformation menu (and then you can transform)
///
///		**WEAPON BRANDISHED**
///		leftshift & rightclick	/ 		lefttrigger & righttrigger	- will/sheath weapon back away
///		leftclick				/		X							- attack (press while holding righttrigger to unleash heavy attack)
///		rightclick				/		righttrigger				- charge heavy attack
///
