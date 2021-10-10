#pragma once

#include <vector>
#include <string>


class GameState
{
public:
	static GameState& getInstance();

	bool						playerIsHoldingWater;
	std::vector<std::string>	playerAllCollectedPuddleGUIDs;
	int							roomEnteringId;
};
