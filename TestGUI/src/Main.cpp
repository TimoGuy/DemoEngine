#include "mainloop/MainLoop.h"

//
// ABOUT THIS PROGRAM
// 
// Specs:
//		MINIMUM:
//			OpenGL 4.5 support (Minimum GTX 740m)
//		RECOMMENDED:
//			GTX 1060 or higher
// 
// Developers:
//		Timothy Bennett
//		Trevor Morris
// 
// Art:
//		ambientCG
//		freePBR
//		Timothy Bennett
// 
// Special Thanks:
//		Arseniy Tepp
//

// NOTE: below is the difference between debug/checked and release.
// With release, we want to disable the console, so subsystem is set to Windows.
#ifdef _DEVELOP
int main()
#else
int __stdcall WinMain(void*, void*, char* cmdLine, int)
#endif
{
	MainLoop::getInstance().initialize();
	MainLoop::getInstance().run();
	MainLoop::getInstance().cleanup();

	return 0;
}
