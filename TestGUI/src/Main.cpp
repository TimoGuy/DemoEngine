#include "RenderEngine/RenderEngine.h"

RenderManager* renderManager;

int main();

int main(void)
{
	renderManager = new RenderManager();
	int exitCode = renderManager->run();
	delete renderManager;
	return 0;
}
