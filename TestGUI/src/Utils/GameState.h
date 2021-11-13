#pragma once

#include <vector>
#include <string>
#include <PxPhysicsAPI.h>


class GameState
{
public:
	static GameState& getInstance();

	physx::PxRigidActor*		playerActorPointer				= nullptr;
	bool						playerIsHoldingWater			= false;
	int							maxPlayerStaminaAmount			= 100;
	int							currentPlayerStaminaAmount		= 46;
	std::vector<std::string>	playerAllCollectedPuddleGUIDs;
	int							roomEnteringId;

	void requestTriggerHold(physx::PxRigidActor* triggerActor);
	void requestTriggerRelease(physx::PxRigidActor* triggerActor);	// NOTE: this hsould remove the triggeractor from the stack... but it's not implemented yet
	physx::PxRigidActor* getCurrentTriggerHold();

private:
	std::vector<physx::PxRigidActor*> currentHeldTriggerActorQueue;
};
