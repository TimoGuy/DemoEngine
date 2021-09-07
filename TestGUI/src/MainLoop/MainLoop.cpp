#include "MainLoop.h"

#include <vector>
#include <thread>
#include <PxPhysicsAPI.h>
#include "../Objects/BaseObject.h"
#include "../RenderEngine/RenderEngine.manager/RenderManager.h"
#include "../RenderEngine/RenderEngine.camera/Camera.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_glfw.h"
#include "../ImGui/imgui_impl_opengl3.h"
#include "../Objects/PlayerCharacter.h"
#include "../RenderEngine/RenderEngine.light/DirectionalLight.h"

#define SINGLE_BUFFERED_MODE 0
#if SINGLE_BUFFERED_MODE
#include <chrono>
#endif


void createWindow(const char* windowName);
void setupViewPort();
void setupImGui();
void setupPhysx();

void physicsUpdate();
void render();


bool loopRunning = false;


void MainLoop::initialize()
{
	createWindow("Test Window");
	setupViewPort();
	setupImGui();
	setupPhysx();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	glClearColor(0.0f, 0.1f, 0.2f, 1.0f);

	glDisable(GL_CULL_FACE);		// Turned off for the pre-processing part, then will be turned on for actual realtime rendering

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//
	// NOTE: activated here so that
	// during the pre-processing steps
	// then culling doesn't hinder it
	//
	/*glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);*/										// NOTE: skybox doesn't render with this on... needs some work.

	renderManager = new RenderManager();

	//
	// Create objects
	//
	PlayerCharacter* player = new PlayerCharacter();
	imguiObjects.push_back(player);
	physicsObjects.push_back(player);
	renderObjects.push_back(player);

	DirectionalLight* light = new DirectionalLight(glm::vec3(0.0f, 90.0f, 0.0f));
	imguiObjects.push_back(light);
	lightObjects.push_back(light);
}

void MainLoop::run()
{
	loopRunning = true;

	std::thread physicsThread(physicsUpdate);			// NOTE: the reason for dispatching a thread to handle the pre-physics handling is to have frame-rate independence. This way, there can be a deltatime calculation happening.
	physicsThread.detach();

	/*// Convert the physx object to model matrix
	glm::mat4 modelMatrix;
	physx::PxTransform t = body->getGlobalPose();
	{
		physx::PxMat44 mat4 = physx::PxMat44(t);
		glm::mat4 newMat;
		newMat[0][0] = mat4[0][0];
		newMat[0][1] = mat4[0][1];
		newMat[0][2] = mat4[0][2];
		newMat[0][3] = mat4[0][3];

		newMat[1][0] = mat4[1][0];
		newMat[1][1] = mat4[1][1];
		newMat[1][2] = mat4[1][2];
		newMat[1][3] = mat4[1][3];

		newMat[2][0] = mat4[2][0];
		newMat[2][1] = mat4[2][1];
		newMat[2][2] = mat4[2][2];
		newMat[2][3] = mat4[2][3];

		newMat[3][0] = mat4[3][0];
		newMat[3][1] = mat4[3][1];
		newMat[3][2] = mat4[3][2];
		newMat[3][3] = mat4[3][3];

		modelMatrix = newMat;
	}*/




	ImGuiIO& io = ImGui::GetIO(); (void)io;

#if SINGLE_BUFFERED_MODE
	const float desiredFrameTime = 1000.0f / 80.0f;
	float startFrameTime = 0;
#endif

	float deltaTime;
	float lastFrame = glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
#if SINGLE_BUFFERED_MODE
		startFrameTime = glfwGetTime();
#endif

		glfwPollEvents();

		if (!io.WantCaptureMouse || ImGui::GetMouseCursor() == ImGuiMouseCursor_None)
			camera.Inputs(window);

		//// Update Animation
		//if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		//	animator.playAnimation(0, 12.0f);
		//if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		//	animator.playAnimation(1, 12.0f);
		//if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		//	animator.playAnimation(2, 12.0f);
		//if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
		//	animator.playAnimation(3, 12.0f);
		//if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
		//	animator.playAnimation(4, 12.0f);
		//if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
		//	animator.playAnimation(5, 12.0f);

		//float currentFrame = glfwGetTime();
		//deltaTime = currentFrame - lastFrame;
		//lastFrame = currentFrame;
		//animator.updateAnimation(deltaTime * deltaTimeMultiplier);
			
		render();

#if SINGLE_BUFFERED_MODE
		glFlush();
		std::this_thread::sleep_for(std::chrono::milliseconds((unsigned int)std::max(0.0, desiredFrameTime - (glfwGetTime() - startFrameTime))));
#else
		glfwSwapBuffers(window);
#endif
	}

	// Signal to physics engine that loop has ended
	loopRunning = false;
}

void MainLoop::cleanup()
{
	delete renderManager;

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	/*glDeleteTextures(1, &texture);
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteProgram(program_id);*/

	glfwDestroyWindow(window);
	glfwTerminate();
}


void createWindow(const char* windowName)
{
	if (!glfwInit())
		glfwTerminate();

#if SINGLE_BUFFERED_MODE
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_FALSE);
	window = glfwCreateWindow(1920, 1080, windowName, NULL, NULL);
#else
	MainLoop::getInstance().window = glfwCreateWindow(1920, 1080, windowName, NULL, NULL);
	glfwSwapInterval(1);
#endif

	if (!MainLoop::getInstance().window)
	{
		glfwTerminate();
	}

	glfwMakeContextCurrent(MainLoop::getInstance().window);
	gladLoadGL();
}


void frameBufferSizeChangedCallback(GLFWwindow* window, int width, int height)
{
	MainLoop::getInstance().camera.width = width;
	MainLoop::getInstance().camera.height = height;
}


void setupViewPort()
{
	int width, height;
	glfwGetFramebufferSize(MainLoop::getInstance().window, &width, &height);
	glfwSetFramebufferSizeCallback(MainLoop::getInstance().window, frameBufferSizeChangedCallback);
	frameBufferSizeChangedCallback(nullptr, width, height);
}


void setupImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsLight();
	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1;
	style.FrameRounding = 1;
	style.WindowTitleAlign.x = 0.5f;
	style.WindowMenuButtonPosition = 1;		// Right side of menu
	style.WindowRounding = 8;

	ImGui_ImplGlfw_InitForOpenGL(MainLoop::getInstance().window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
}


physx::PxFilterFlags contactReportFilterShader(physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
	physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
	physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize)
{
	PX_UNUSED(attributes0);
	PX_UNUSED(attributes1);
	PX_UNUSED(filterData0);
	PX_UNUSED(filterData1);
	PX_UNUSED(constantBlockSize);
	PX_UNUSED(constantBlock);

	//
	// Enable CCD for the pair, request contact reports for initial and CCD contacts.
	// Additionally, provide information per contact point and provide the actor
	// pose at the time of contact.
	//

	pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT
		| physx::PxPairFlag::eDETECT_CCD_CONTACT
		| physx::PxPairFlag::eNOTIFY_TOUCH_CCD
		| physx::PxPairFlag::eNOTIFY_TOUCH_FOUND
		| physx::PxPairFlag::eNOTIFY_CONTACT_POINTS
		| physx::PxPairFlag::eCONTACT_EVENT_POSE;
	return physx::PxFilterFlag::eDEFAULT;
}


std::vector<physx::PxVec3> gContactPositions;
std::vector<physx::PxVec3> gContactImpulses;
std::vector<physx::PxVec3> gContactSphereActorPositions;

class ContactReportCallback : public physx::PxSimulationEventCallback
{
	void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) { PX_UNUSED(constraints); PX_UNUSED(count); }
	void onWake(physx::PxActor** actors, physx::PxU32 count) { PX_UNUSED(actors); PX_UNUSED(count); }
	void onSleep(physx::PxActor** actors, physx::PxU32 count) { PX_UNUSED(actors); PX_UNUSED(count); }
	void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) { PX_UNUSED(pairs); PX_UNUSED(count); }
	void onAdvance(const physx::PxRigidBody* const*, const physx::PxTransform*, const physx::PxU32) {}
	void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
	{
		std::vector<physx::PxContactPairPoint> contactPoints;

		physx::PxTransform spherePose(physx::PxIdentity);
		physx::PxU32 nextPairIndex = 0xffffffff;

		physx::PxContactPairExtraDataIterator iter(pairHeader.extraDataStream, pairHeader.extraDataStreamSize);
		bool hasItemSet = iter.nextItemSet();
		if (hasItemSet)
			nextPairIndex = iter.contactPairIndex;

		for (physx::PxU32 i = 0; i < nbPairs; i++)
		{
			//
			// Get the pose of the dynamic object at time of impact.
			//
			if (nextPairIndex == i)
			{
				if (pairHeader.actors[0]->is<physx::PxRigidDynamic>())
					spherePose = iter.eventPose->globalPose[0];
				else
					spherePose = iter.eventPose->globalPose[1];

				gContactSphereActorPositions.push_back(spherePose.p);

				hasItemSet = iter.nextItemSet();
				if (hasItemSet)
					nextPairIndex = iter.contactPairIndex;
			}

			//
			// Get the contact points for the pair.
			//
			const physx::PxContactPair& cPair = pairs[i];
			if (cPair.events & (physx::PxPairFlag::eNOTIFY_TOUCH_FOUND | physx::PxPairFlag::eNOTIFY_TOUCH_CCD))
			{
				physx::PxU32 contactCount = cPair.contactCount;
				contactPoints.resize(contactCount);
				cPair.extractContacts(&contactPoints[0], contactCount);

				for (physx::PxU32 j = 0; j < contactCount; j++)
				{
					gContactPositions.push_back(contactPoints[j].position);
					gContactImpulses.push_back(contactPoints[j].impulse);
				}
			}
		}
	}
};

ContactReportCallback gContactReportCallback;
physx::PxScene* gScene = NULL;
physx::PxDefaultErrorCallback gErrorCallback;
physx::PxDefaultAllocator gAllocator;
physx::PxDefaultCpuDispatcher* gDispatcher = NULL;
physx::PxTolerancesScale tolerancesScale;

physx::PxFoundation* gFoundation = NULL;
physx::PxPhysics* gPhysics = NULL;
physx::PxCooking* gCooking = NULL;

physx::PxMaterial* gMaterial = NULL;
physx::PxRigidStatic* gTriangleMeshActor = NULL;
physx::PxRigidDynamic* gSphereActor = NULL;
physx::PxPvd* gPvd = NULL;


physx::PxRigidDynamic* body = NULL;

void setupPhysx()
{



	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
	gPvd = PxCreatePvd(*gFoundation);
	physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
	gPvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

	tolerancesScale.length = 100;
	tolerancesScale.speed = 981;

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, tolerancesScale, true, gPvd);

	const int numCores = 4;
	gDispatcher = physx::PxDefaultCpuDispatcherCreate(numCores == 0 ? 0 : numCores - 1);
	physx::PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.gravity = physx::PxVec3(0, -9.81f, 0);
	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader; // contactReportFilterShader;
	sceneDesc.simulationEventCallback = &gContactReportCallback;
	sceneDesc.flags |= physx::PxSceneFlag::eENABLE_CCD;
	sceneDesc.ccdMaxPasses = 4;



	gScene = gPhysics->createScene(sceneDesc);
	/*physx::PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
	if (pvdClient)
	{
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
	}*/
	gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 1.0f);



	physx::PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
	if (pvdClient)
	{
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}


	//
	// Init scene
	//
	physx::PxRigidStatic* groundPlane = physx::PxCreatePlane(*gPhysics, physx::PxPlane(0, 1, 0, 3.6f), *gMaterial);
	gScene->addActor(*groundPlane);

	float halfExtent = 1.0f;
	const int size = 50;
	physx::PxTransform t;
	physx::PxShape* shape = gPhysics->createShape(physx::PxBoxGeometry(halfExtent, halfExtent, halfExtent), *gMaterial);
	/*for (physx::PxU32 i = 0; i < size; i++)
	{
		for (physx::PxU32 j = 0; j < size - i; j++)
		{
			physx::PxTransform localTm(physx::PxVec3(physx::PxReal(j * 2) - physx::PxReal(size - i), physx::PxReal(i * 5 + 1), 0) * halfExtent);
			physx::PxRigidDynamic* body = gPhysics->createRigidDynamic(localTm);
			body->attachShape(*shape);

			physx::PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
			gScene->addActor(*body);
		}
	}*/

	physx::PxTransform localTm(physx::PxVec3(physx::PxReal(0), physx::PxReal(200), 0) * halfExtent);
	body = gPhysics->createRigidDynamic(localTm);
	body->attachShape(*shape);

	physx::PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
	gScene->addActor(*body);
	shape->release();
}


void physicsUpdate()
{
	while (loopRunning)
	{
		float deltaTime = 1.0f / 50.0f;

		for (unsigned int i = 0; i < MainLoop::getInstance().physicsObjects.size(); i++)
		{
			MainLoop::getInstance().physicsObjects[i]->physicsUpdate(deltaTime);
		}

		gScene->simulate(deltaTime);
		gScene->fetchResults(true);
	}
}


void render()
{
	//
	// Render out the rendermanager
	//
	MainLoop::getInstance().renderManager->render();
}