#include "MainLoop/MainLoop.h"


int main()
{
	MainLoop::initialize();
	MainLoop::run();
	MainLoop::cleanup();

	return 0;
}
