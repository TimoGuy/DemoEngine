#include "mainloop/MainLoop.h"


int main()
{
	MainLoop::getInstance().initialize();
	MainLoop::getInstance().run();
	MainLoop::getInstance().cleanup();

	return 0;
}
