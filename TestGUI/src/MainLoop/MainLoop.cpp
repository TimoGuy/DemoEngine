#include "MainLoop.h"

#include <vector>
#include <thread>
#include <glad/glad.h>
#include "../objects/BaseObject.h"
#include "../render_engine/render_manager/RenderManager.h"
#include "../render_engine/camera/Camera.h"

#ifdef _DEBUG
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"
#include "../imgui/ImGuizmo.h"
#endif

#include "../utils/InputManager.h"
#include "../utils/FileLoading.h"


#define FULLSCREEN_MODE 0
#define SINGLE_BUFFERED_MODE 0
#if SINGLE_BUFFERED_MODE
#include <chrono>
#endif


void createWindow(const char* windowName);
void setupViewPort();
void setupPhysx();

#ifdef _DEBUG
void setupImGui();
#endif

void physicsUpdate();

#ifdef _DEBUG
void GLAPIENTRY openglMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
		return;

	fprintf(
		stderr,
		"GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type,
		severity,
		message
	);
}
#endif


MainLoop& MainLoop::getInstance()
{
	static MainLoop instance;
	return instance;
}

void MainLoop::initialize()
{
	// https://www.text-image.com/convert/ascii.html
	const char* logoText =
		"    `-`         .`         `.`                                                      `s+               \n"
		"    yNo     .+hmmh sm:  -sdNm+      :+/osyyyys+-    `ss+oyyyyyo/`       ````````````-MN`````````````  \n"
		" ```hMs``` -mNs:.` hMs oNmo-`       mMyso++++sdNh`  /Mmoso+++oymm/    .mmmmmmmmmmmmmNMMmmmmmmmmmmmmmd`\n"
		"+mmmNMNmmm.+Md. .- yMd dMy  -.     `MM-`````  `hMo  oMh``````  .NN-    .........-://oMM///:--........ \n"
		"`...hMy... `sNNdNd.oMN -hNddNy     .MMmmmmmmmy sMy  sMNmmmmmmms yMo        `:oydmddddMMddddmmho:`     \n"
		"    dMs     :NMh:. -MM- +MNy-.     :MN-------. hMo  oMh-------. +Mh       +mNdo:-..`-MN``...:+hNm/    \n"
		"   oMMh.   :NN/`/ms`mMo+Mm- ym/    /MN::::::::/NM.  /Md:::::::::sMh      oMm:`      .MN        -dMo   \n"
		"  oMMMMms- +MN+/sMMs+MmyMh//hMN:   /MNhhhhhhhhhy+   .hyhhhhhhhhhmMh     `NM- ```````-MN```````` -MN`  \n"
		" sMddMyodm-`/yhhyshm:mMooyhhyymh   /Mh        oo`    `+s`       oMy     -MN odddddddmMMdddddddh` NM-  \n"
		"sMh`hMs `.`/++++++os+hMm+++:-dh.   /Md       `sMd-  /mNs`       yMs     .MM``-------/MN-------- .MM.  \n"
		"ss` hMs    shMMmhhhhhhdMMhho`yMh`  /Md     /hhdNMmddNMNdh/      dM+      hMs`       .MN        `yMy   \n"
		"    hMs     +Mm.      `dMo `:-+y.  :Mm     .:::::sMNo::::.      NM-      .hNd+-``   .MN    `.-+dNy`   \n"
		"    hMs    .NMmyo:.    .dMshMy     -MN     `....oNN+.....`     .MM`        :sdmdhhhyhMMyyhhddmds-     \n"
		"    hMs    hMy+shmmy`   -NMMy`     .MM     smmmNMNNNNNmmms     /Mm      ``````-:/++osMMoo+//:.``````  \n"
		"    hMs   :MN`   `::  .omNhNN+`     MM.    `.-hMh.-smMd+.`     sMy     `hddddddddddddMMddddddddddddh` \n"
		"    yMo   sNs         dmo- .yNh`    mM-      :Nd`   `/hm.      dM/      ------------/MN-------------  \n"
		"    .:`   `:`                --     :o`       .`               /o`                  `ys               \n";


	std::cout << logoText << std::endl;

	createWindow("Test Window");
	setupViewPort();
	setupPhysx();

#ifdef _DEBUG
	setupImGui();
#endif

	std::cout << "OpenGL Version and Vendor:\t" << glGetString(GL_VERSION) << std::endl << std::endl;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

#ifdef _DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(openglMessageCallback, 0);
#endif

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
	// Once loading of all the internals happens, now we can load in the level
	//
	FileLoading::getInstance().loadFileWithPrompt();
}


void MainLoop::run()
{
#ifdef _DEBUG
	ImGuiIO& io = ImGui::GetIO(); (void)io;
#endif

#if SINGLE_BUFFERED_MODE
	const float desiredFrameTime = 1000.0f / 60.0f;
	float startFrameTime = 0;
#endif

	float lastFrame = (float)glfwGetTime();
	float accumulatedTimeForPhysics = 0.0f;		// NOTE: https://gafferongames.com/post/fix_your_timestep/
	this->physicsDeltaTime = 0.02f;

	while (!glfwWindowShouldClose(window))
	{
#if SINGLE_BUFFERED_MODE
		startFrameTime = (float)glfwGetTime();
#endif

		glfwPollEvents();

#ifdef _DEBUG
		//
		// ImGui render pass
		//
		int prevImGuiMouseCursor = ImGui::GetMouseCursor();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::BeginFrame();

		if (!io.WantCaptureMouse || prevImGuiMouseCursor == ImGuiMouseCursor_None)
		{
			static bool prevLeftMouseButtonPressed = false;
			if (!prevLeftMouseButtonPressed && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
			{
				renderManager->flagForPicking();
			}
			prevLeftMouseButtonPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
#endif
			camera.Inputs(window);
#ifdef _DEBUG
		}
#endif

		//
		// Update the input manager
		//
		InputManager::getInstance().updateInputState();

		//
		// Update deltaTime for rendering
		//
		float currentFrame = (float)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		if (deltaTime > 0.1f)
			deltaTime = 0.1f;		// Clamp it if it's too big! (10 fps btw) (1/10 = 0.1)
		lastFrame = currentFrame;

		//
		// Run Physics intermittently
		// 
		// NOTE: https://gafferongames.com/post/fix_your_timestep/
		//
		accumulatedTimeForPhysics += deltaTime;
		while (accumulatedTimeForPhysics >= this->physicsDeltaTime)
		{
			physicsUpdate();
			accumulatedTimeForPhysics -= this->physicsDeltaTime;
		}

		//
		// Take all physics objects and update their transforms (interpolation)
		//
		float interpolationAlpha = accumulatedTimeForPhysics / this->physicsDeltaTime;
#ifdef _DEBUG
		if (!playMode)
			interpolationAlpha = 1.0f;
#endif
		for (size_t i = 0; i < physicsObjects.size(); i++)
		{
			physicsObjects[i]->baseObject->INTERNALfetchInterpolatedPhysicsTransform(interpolationAlpha);
		}
			
		//
		// Do all pre-render updates
		//
		for (size_t i = 0; i < objects.size(); i++)
		{
			objects[i]->preRenderUpdate();
		}

		// Update camera after all other updates
		camera.updateToVirtualCameras();

		//
		// Render out the rendermanager
		//
		renderManager->render();

//#if SINGLE_BUFFERED_MODE
//		glFlush();
//		//float haha = (glfwGetTime() - startFrameTime) * 1000.0;
//		unsigned int sleepAmount = (unsigned int)std::max(0.0, desiredFrameTime - (glfwGetTime() - startFrameTime) * 1000.0);
//		std::this_thread::sleep_for(std::chrono::milliseconds(sleepAmount));		// TODO: comment this out if wanted
//		//std::cout << "jfjlskdjf\n";
//#else
		glfwSwapBuffers(window);
//#endif
	}
}

void MainLoop::cleanup()
{
	delete renderManager;

#ifdef _DEBUG
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
#endif

	glfwDestroyWindow(window);
	glfwTerminate();
}


void createWindow(const char* windowName)
{
	if (!glfwInit())
		glfwTerminate();

#if FULLSCREEN_MODE
	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

	MainLoop::getInstance().window = glfwCreateWindow(mode->width, mode->height, windowName, primaryMonitor, NULL);
#else
	MainLoop::getInstance().window = glfwCreateWindow(1920, 1080, windowName, NULL, NULL);
#endif

	if (!MainLoop::getInstance().window)
	{
		glfwTerminate();
	}

	glfwMakeContextCurrent(MainLoop::getInstance().window);

	// NOTE: glfwswapinterval() needs to be executed after glfwmakecontextcurrent()
#if SINGLE_BUFFERED_MODE
	glfwSwapInterval(0);
#else
	glfwSwapInterval(1);
#endif

	gladLoadGL();
}


void frameBufferSizeChangedCallback(GLFWwindow* window, int width, int height)
{
	if (width == 0 || height == 0)
		return;

	MainLoop::getInstance().camera.width = (float)width;
	MainLoop::getInstance().camera.height = (float)height;

	if (MainLoop::getInstance().renderManager != nullptr)
		MainLoop::getInstance().renderManager->recreateRenderBuffers();
}


void setupViewPort()
{
	int width, height;
	glfwGetFramebufferSize(MainLoop::getInstance().window, &width, &height);
	glfwSetFramebufferSizeCallback(MainLoop::getInstance().window, frameBufferSizeChangedCallback);
	frameBufferSizeChangedCallback(nullptr, width, height);
}

#ifdef _DEBUG
void setupImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();
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
#endif


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
	void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
	{
		for (physx::PxU32 i = 0; i < count; i++)
		{
			// Ignore pairs when shapes have been deleted
			if (pairs[i].flags & (physx::PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | physx::PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
				continue;

			//
			// Find which actor is needing this callback!!!
			// @NOTE: the onTrigger() function will call either when enter or leave the trigger, so keep that PxTriggerPair ref handy
			//
			for (size_t ii = 0; ii < MainLoop::getInstance().physicsObjects.size(); ii++)
			{
				if (pairs[i].triggerActor == MainLoop::getInstance().physicsObjects[ii]->getActor())
				{
					MainLoop::getInstance().physicsObjects[ii]->INTERNALonTrigger(pairs[i]);
				}
			}
		}
	}
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
physx::PxDefaultErrorCallback gErrorCallback;
physx::PxDefaultAllocator gAllocator;
physx::PxDefaultCpuDispatcher* gDispatcher = NULL;
physx::PxTolerancesScale tolerancesScale;

physx::PxFoundation* gFoundation = NULL;

physx::PxRigidStatic* gTriangleMeshActor = NULL;
physx::PxRigidDynamic* gSphereActor = NULL;
physx::PxPvd* gPvd = NULL;

void setupPhysx()
{
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

	gPvd = PxCreatePvd(*gFoundation);
	physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
	gPvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

	tolerancesScale.length = 100;
	tolerancesScale.speed = 981;

	MainLoop::getInstance().physicsPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, tolerancesScale, true, gPvd);

	const int numCores = 4;
	gDispatcher = physx::PxDefaultCpuDispatcherCreate(numCores == 0 ? 0 : numCores - 1);
	physx::PxSceneDesc sceneDesc(MainLoop::getInstance().physicsPhysics->getTolerancesScale());
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.gravity = physx::PxVec3(0, -98.1f, 0);
	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader; // contactReportFilterShader;
	sceneDesc.simulationEventCallback = &gContactReportCallback;
	sceneDesc.flags |= physx::PxSceneFlag::eENABLE_CCD;
	sceneDesc.ccdMaxPasses = 4;

	MainLoop::getInstance().physicsCooking = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, physx::PxCookingParams(tolerancesScale));
	if (!MainLoop::getInstance().physicsCooking)
		std::cerr << "FATAL ERROR::::PxCreateCooking failed!" << std::endl;

	MainLoop::getInstance().physicsScene = MainLoop::getInstance().physicsPhysics->createScene(sceneDesc);
	MainLoop::getInstance().defaultPhysicsMaterial = MainLoop::getInstance().physicsPhysics->createMaterial(0.5f, 0.5f, 1.0f);

#ifdef _DEBUG
	MainLoop::getInstance().physicsScene->setVisualizationParameter(physx::PxVisualizationParameter::eSCALE, 1.0f);
	MainLoop::getInstance().physicsScene->setVisualizationParameter(physx::PxVisualizationParameter::eACTOR_AXES, 2.0f);
	MainLoop::getInstance().physicsScene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_SHAPES, true);
#endif


	physx::PxPvdSceneClient* pvdClient = MainLoop::getInstance().physicsScene->getScenePvdClient();
	if (pvdClient)
	{
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	//
	// Create controller manager
	//
	MainLoop::getInstance().physicsControllerManager = PxCreateControllerManager(*MainLoop::getInstance().physicsScene);
}


void physicsUpdate()
{
#ifdef _DEBUG
	// Don't run unless if play mode
	if (!MainLoop::getInstance().playMode)
		return;
#endif

	for (unsigned int i = 0; i < MainLoop::getInstance().physicsObjects.size(); i++)
	{
		MainLoop::getInstance().physicsObjects[i]->physicsUpdate();
	}

	MainLoop::getInstance().physicsScene->simulate(MainLoop::getInstance().physicsDeltaTime);
	MainLoop::getInstance().physicsScene->fetchResults(true);
			
#ifdef _DEBUG
	// Don't visualize unless enabled
	if (!RenderManager::renderPhysicsDebug)
		return;
	
	const physx::PxRenderBuffer& rb = MainLoop::getInstance().physicsScene->getRenderBuffer();
	const physx::PxU32 numLines = rb.getNbLines();
	std::vector<physx::PxDebugLine>* lineList = new std::vector<physx::PxDebugLine>();

	for (physx::PxU32 i = 0; i < numLines; i++)
	{
		const physx::PxDebugLine& line = rb.getLines()[i];
		lineList->push_back(line);
	}
	MainLoop::getInstance().renderManager->physxVisSetDebugLineList(lineList);
#endif
}
