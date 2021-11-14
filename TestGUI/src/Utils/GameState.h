#pragma once

#include <vector>
#include <string>
#include <PxPhysicsAPI.h>


enum StaminaEvent
{
	JUMP, MOVE
};

class GameState
{
public:
	static GameState& getInstance();

	physx::PxRigidActor*		playerActorPointer				= nullptr;
	bool						playerIsHoldingWater			= false;
	int							maxPlayerStaminaAmount			= 100;
	float						currentPlayerStaminaAmount		= 100;
	float						playerStaminaDepleteChaser		= 100;		// NOTE: this shows how much the stamina is depleting, so it will essentially follow the currentPlayerStaminaAmount
	std::vector<std::string>	playerAllCollectedPuddleGUIDs;
	int							roomEnteringId;

	void requestTriggerHold(physx::PxRigidActor* triggerActor);
	void requestTriggerRelease(physx::PxRigidActor* triggerActor);	// NOTE: this hsould remove the triggeractor from the stack... but it's not implemented yet
	physx::PxRigidActor* getCurrentTriggerHold();

	void inputStaminaEvent(StaminaEvent staminaEvent, float deltaTime = 1.0f);
	void updateStaminaDepletionChaser(float deltaTime = 1.0f);

private:
	std::vector<physx::PxRigidActor*> currentHeldTriggerActorQueue;
};
