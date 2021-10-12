#include "GameState.h"

#include <iostream>


GameState& GameState::getInstance()
{
	static GameState instance;
	return instance;
}

void GameState::requestTriggerHold(physx::PxActor* triggerActor)
{
	currentHeldTriggerActor = triggerActor;
}

void GameState::requestTriggerRelease(physx::PxActor* triggerActor)
{
	std::cout << "Not implemented yet" << std::endl;
}

physx::PxActor* GameState::getCurrentTriggerHold()
{
	return currentHeldTriggerActor;
}
