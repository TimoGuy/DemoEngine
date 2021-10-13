#include "GameState.h"

#include <iostream>


GameState& GameState::getInstance()
{
	static GameState instance;
	return instance;
}

void GameState::requestTriggerHold(physx::PxRigidActor* triggerActor)
{
	currentHeldTriggerActor = triggerActor;
}

void GameState::requestTriggerRelease(physx::PxRigidActor* triggerActor)
{
	std::cout << "Not implemented yet" << std::endl;

	// TEMP remove from "stack"
	if (triggerActor == currentHeldTriggerActor)
		currentHeldTriggerActor = nullptr;
}

physx::PxRigidActor* GameState::getCurrentTriggerHold()
{
	return currentHeldTriggerActor;
}
