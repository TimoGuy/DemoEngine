#include "GameLoop/GameLoop.h"


int main(void)
{
	// WARNING: a lot can go wrong in these next three lines
	GameLoop::initialize();
	GameLoop::run();
	GameLoop::destroy();
	return 0;
}
