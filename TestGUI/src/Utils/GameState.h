#pragma once

#include <vector>
#include <string>
#include <PxPhysicsAPI.h>


class GameState
{
public:
	static GameState& getInstance();

	physx::PxActor*				playerActorPointer					= nullptr;
	bool						playerIsHoldingWater;
	std::vector<std::string>	playerAllCollectedPuddleGUIDs;
	int							roomEnteringId;

	void requestTriggerHold(physx::PxActor* triggerActor);
	void requestTriggerRelease(physx::PxActor* triggerActor);	// NOTE: this hsould remove the triggeractor from the stack... but it's not implemented yet
	physx::PxActor* getCurrentTriggerHold();

private:
	physx::PxActor* currentHeldTriggerActor = nullptr;		// @TODO: this needs to be a stack. And then getCurrentTriggerHold() returns the most recent thing on the stack
};
