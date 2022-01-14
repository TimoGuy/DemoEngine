#include "mainloop/MainLoop.h"

//
// ABOUT THIS PROGRAM
// 
// Recommended Specs:
//		MINIMUM:
//			OpenGL 4.5 support (Minimum GTX 600 series and Radeon HD 7000 series)
//			https://www.reddit.com/r/opengl/comments/edw1sh/graphics_card_that_supports_opengl_45/
// 
//			Intel® HD Graphics 520/530 or Intel® Iris® Plus Graphics 540 / 550 / 580
//			https://www.intel.com/content/www/us/en/support/articles/000005524/graphics.html
// 
//		RECOMMENDED:
//			GTX 1060 or higher
//			Seems like for AMD side the Radeon RX 5600XT is higher than the 1060 and the Radeon RX580 is lower
//			https://www.videocardbenchmark.net/common_gpus.html
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
