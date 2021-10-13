#include "GameState.h"

#include <iostream>


GameState& GameState::getInstance()
{
	static GameState instance;
	return instance;
}

void GameState::requestTriggerHold(physx::PxRigidActor* triggerActor)
{
	// Add at the end of the queue
	currentHeldTriggerActorQueue.push_back(triggerActor);
}

void GameState::requestTriggerRelease(physx::PxRigidActor* triggerActor)
{
	// Remove from queue
	currentHeldTriggerActorQueue.erase(std::remove(currentHeldTriggerActorQueue.begin(), currentHeldTriggerActorQueue.end(), triggerActor), currentHeldTriggerActorQueue.end());
}

physx::PxRigidActor* GameState::getCurrentTriggerHold()
{
	if (currentHeldTriggerActorQueue.size() == 0)
		return nullptr;

	// Grab the beginning of the queue
	return currentHeldTriggerActorQueue[0];
}
