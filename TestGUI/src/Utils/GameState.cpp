#include "GameState.h"

#include "PhysicsUtils.h"
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

void GameState::inputStaminaEvent(StaminaEvent staminaEvent, float deltaTime)
{
	switch (staminaEvent)
	{
	case StaminaEvent::JUMP:
		currentPlayerStaminaAmount -= 1.0f;
		std::cout << "Jumped!" << std::endl;		// TODO: there's a problem here: the player jumps twice sometimes
		break;

	case StaminaEvent::MOVE:
		currentPlayerStaminaAmount -= 1.0f * deltaTime;
		break;

	default:
		std::cout << "Stamina Event " << staminaEvent << " unknown." << std::endl;
		break;
	}
}

void GameState::updateStaminaDepletionChaser(float deltaTime)
{
	playerStaminaDepleteChaser = PhysicsUtils::moveTowards(playerStaminaDepleteChaser, currentPlayerStaminaAmount, deltaTime);
}
