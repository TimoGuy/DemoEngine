#include "RenderEngine/RenderEngine.h"

RenderManager* renderManager;

int main();

int main(void)
{
	renderManager = new RenderManager();
	float vertices[] = {
		-0.5, 0.5, 0.0, // v0
		-0.5, -0.5, 0.0, // v1
		0.5, 0.5, 0.0, // v2
		0.5, -0.5, 0.0, // v3
		0.5, 0.5, 0.0, // v2
		-0.5, -0.5, 0.0 // v1
	};

	float normals[] = { 0.0 };
	RenderableObject* rect = new RenderableObject(0, vertices, normals);
	std::vector<RenderableObject>* list = renderManager->getRenderList();
	list->push_back(*rect);
	renderManager->run();

	return 0;
}