#include "GameState.h"

#include "PhysicsUtils.h"
#include <iostream>


GameState& GameState::getInstance()
{
	static GameState instance;
	return instance;
}

void GameState::addPuddleGUID(const std::string& guid)
{
	playerAllCollectedPuddleGUIDs.push_back(guid);
}

void GameState::removePuddleGUID(const std::string& guid)
{
	playerAllCollectedPuddleGUIDs.erase(
		std::remove(
			playerAllCollectedPuddleGUIDs.begin(),
			playerAllCollectedPuddleGUIDs.end(),
			guid
		),
		playerAllCollectedPuddleGUIDs.end()
	);
}

bool GameState::isPuddleCollected(const std::string& guid)
{
	return (std::find(playerAllCollectedPuddleGUIDs.begin(), playerAllCollectedPuddleGUIDs.end(), guid) != playerAllCollectedPuddleGUIDs.end());
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
	float staminaDepletionMultiplier = 0.2f;

	switch (staminaEvent)
	{
	case StaminaEvent::JUMP:
		currentPlayerStaminaAmount -= 1.0f * staminaDepletionMultiplier;
		std::cout << "Jumped!" << std::endl;		// TODO: there's a problem here: the player jumps twice sometimes
		break;

	case StaminaEvent::MOVE:
		currentPlayerStaminaAmount -= 1.0f * staminaDepletionMultiplier * deltaTime;
		break;

	default:
		int toInt = (int)staminaEvent;
		std::cout << "Stamina Event " << toInt << " unknown." << std::endl;
		break;
	}

	staminaWasDepletedThisFrame = true;
}

void GameState::updateStaminaDepletionChaser(float deltaTime)
{
	//
	// Calculate the depletion speed (visual effect)
	//
	float lerpSpeed = std::abs(playerStaminaDepleteChaser - currentPlayerStaminaAmount) * 0.02f;
	float speedingUpSpeed = staminaDepletionSpeed + staminaDepletionAccel * deltaTime;
	staminaDepletionSpeed = std::min(lerpSpeed, speedingUpSpeed);

	//
	// Do the depletion calc
	//
	static const float barPadding = 0.25f;
	playerStaminaDepleteChaser =
		PhysicsUtils::moveTowards(playerStaminaDepleteChaser, currentPlayerStaminaAmount + (staminaWasDepletedThisFrame ? barPadding : 0.0f), staminaDepletionSpeed * 60.0f * deltaTime);
	staminaWasDepletedThisFrame = false;
}

void GameState::updateDayNightTime(float deltaTime)
{
	if (!isDayNightTimeMoving)
		return;

	dayNightTime += dayNightTimeSpeed * deltaTime;
}
