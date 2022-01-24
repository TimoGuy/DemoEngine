#include "MainLoop.h"

#include <vector>
#include <thread>
#include <glad/glad.h>
#include "../objects/BaseObject.h"
#include "../render_engine/render_manager/RenderManager.h"
#include "../render_engine/material/Texture.h"
#include "../render_engine/camera/Camera.h"

#ifdef _DEVELOP
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"
#include "../imgui/ImGuizmo.h"
#endif

#include "../utils/InputManager.h"
#include "../utils/FileLoading.h"
#include "../utils/GameState.h"
#include "../render_engine/resources/Resources.h"


#define FULLSCREEN_MODE 0
#define V_SYNC 1


void createWindow(const char* appName);
void setupViewPort();
void setupPhysx();

#ifdef _DEVELOP
void setupImGui();
#endif

void physicsUpdate();

#ifdef _DEVELOP
void GLAPIENTRY openglMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	auto const src_str = [source]() {
		switch (source)
		{
		case GL_DEBUG_SOURCE_API: return "API";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW SYSTEM";
		case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
		case GL_DEBUG_SOURCE_THIRD_PARTY: return "THIRD PARTY";
		case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
		case GL_DEBUG_SOURCE_OTHER: return "OTHER";
		}
	}();

	auto const type_str = [type]() {
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR: return "ERROR";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED_BEHAVIOR";
		case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
		case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
		case GL_DEBUG_TYPE_MARKER: return "MARKER";
		case GL_DEBUG_TYPE_OTHER: return "OTHER";
		}
	}();

	auto const severity_str = [severity]() {
		switch (severity) {
		case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
		case GL_DEBUG_SEVERITY_LOW: return "LOW";
		case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
		case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
		}
	}();
	std::cout << src_str << ", " << type_str << ", " << severity_str << ", " << id << ": " << message << '\n';
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

	createWindow("solanine prealpha");
	setupViewPort();
	setupPhysx();

#ifdef _DEVELOP
	setupImGui();
#endif

	int maxUBOBinds, maxUBOSize, maxTexSize, maxArrayLayers, max3DTexSize, maxTexUnits;
	glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUBOBinds);
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
	glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxArrayLayers);
	glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3DTexSize);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTexUnits);
	int workGroupSizes[3] = { 0 };
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &workGroupSizes[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &workGroupSizes[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &workGroupSizes[2]);
	int workGroupCounts[3] = { 0 };
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &workGroupCounts[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &workGroupCounts[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &workGroupCounts[2]);

	std::cout << "Graphics Renderer:\t" << glGetString(GL_RENDERER) << std::endl;
	std::cout << "OpenGL Version:\t\t" << glGetString(GL_VERSION) << std::endl;
	std::cout << std::endl;
	std::cout << "GL_MAX_UNIFORM_BUFFER_BINDINGS:\t\t" << maxUBOBinds << std::endl;
	std::cout << "GL_MAX_UNIFORM_BLOCK_SIZE:\t\t" << maxUBOSize << std::endl;
	std::cout << "GL_MAX_TEXTURE_SIZE:\t\t\t" << maxTexSize << std::endl;
	std::cout << "GL_MAX_ARRAY_TEXTURE_LAYERS:\t\t" << maxArrayLayers << std::endl;
	std::cout << "GL_MAX_3D_TEXTURE_SIZE:\t\t\t" << max3DTexSize << std::endl;
	std::cout << "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:\t" << maxTexUnits << std::endl;
	std::cout << "GL_MAX_COMPUTE_WORK_GROUP_SIZE:\t\t{ " << workGroupSizes[0] << ", " << workGroupSizes[1] << ", " << workGroupSizes[2] << "}" << std::endl;
	std::cout << "GL_MAX_COMPUTE_WORK_GROUP_COUNT:\t{ " << workGroupCounts[0] << ", " << workGroupCounts[1] << ", " << workGroupCounts[2] << "}" << std::endl;
	std::cout << std::endl;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

#ifdef _DEVELOP
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
	FileLoading::getInstance().loadFileWithPrompt(false);
}


void MainLoop::run()
{
#ifdef _DEVELOP
	ImGuiIO& io = ImGui::GetIO(); (void)io;
#endif

	float lastFrame = (float)glfwGetTime();
	float accumulatedTimeForPhysics = 0.0f;		// NOTE: https://gafferongames.com/post/fix_your_timestep/
	this->physicsDeltaTime = 0.02f;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		//
		// Switch fullscreen or not when pressing ALT+ENTER
		//
		{
			static bool isAltPressed = false;
			static bool isEnterPressed = false;
			static bool isKeyComboPressedPrev = false;

			isAltPressed = (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS);
			isEnterPressed = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
			if (isAltPressed && isEnterPressed)
			{
				if (!isKeyComboPressedPrev)
				{
					setFullscreen(!isFullscreen);
				}

				isKeyComboPressedPrev = true;
			}
			else
				isKeyComboPressedPrev = false;
		}

#ifdef _DEVELOP
		//
		// ImGui render pass
		//
		int prevImGuiMouseCursor = ImGui::GetMouseCursor();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::AllowAxisFlip(false);
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
#ifdef _DEVELOP
		}
#endif

		//
		// Load async loaded resources to GPU
		//
		Texture::INTERNALtriggerCreateGraphicsAPITextureHandles();

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
#ifdef _DEVELOP
		if (!playMode)
			interpolationAlpha = 1.0f;
#endif
		for (size_t i = 0; i < physicsObjects.size(); i++)
		{
			physicsObjects[i]->baseObject->INTERNALfetchInterpolatedPhysicsTransform(interpolationAlpha);
		}

		// Update time of day
		GameState::getInstance().updateDayNightTime(deltaTime);

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

		//
		// Delete all objects that were marked for deletion
		//
		for (int i = objectsToDelete.size() - 1; i >= 0; i--)
		{
			delete objectsToDelete[i];
		}
		objectsToDelete.clear();

		// SWAP DEM BUFFERS
		glfwSwapBuffers(window);
	}
}

void MainLoop::cleanup()
{
	delete renderManager;

#ifdef _DEVELOP
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
#endif

	glfwDestroyWindow(window);
	glfwTerminate();
}

void MainLoop::deleteObject(BaseObject* obj)
{
	// Don't add unless not in the deletion list
	if (std::find(objectsToDelete.begin(), objectsToDelete.end(), obj) != objectsToDelete.end())
		return;

	objectsToDelete.push_back(obj);
}

void MainLoop::setFullscreen(bool flag, bool force)
{
	if (!force && isFullscreen == flag)
		return;

	isFullscreen = flag;

	if (isFullscreen)
	{
		// Save cache
		glfwGetWindowPos(window, &windowXposCache, &windowYposCache);
		glfwGetWindowSize(window, &windowWidthCache, &windowHeightCache);

		// Make fullscreen
		GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
		glfwSetWindowMonitor(window, primaryMonitor, 0, 0, mode->width, mode->height, mode->refreshRate);
	}
	else
	{
		// Make windowed
		GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
		glfwSetWindowMonitor(window, NULL, windowXposCache, windowYposCache, windowWidthCache, windowHeightCache, mode->refreshRate);
	}

	//
	// Make sure vsync is on
	//
	glfwMakeContextCurrent(window);
#if V_SYNC
	glfwSwapInterval(1);
#else
	glfwSwapInterval(0);
#endif
}


void createWindow(const char* appName)
{
	if (!glfwInit())
		glfwTerminate();

	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

#if _DEBUG
#define BUILD_VERSION "debug"
#elif _DEVELOP
#define BUILD_VERSION "checked"
#else
#define BUILD_VERSION "release"
#endif

	// https://stackoverflow.com/questions/17739390/different-format-of-date-macro
	constexpr unsigned int compileYear = (__DATE__[7] - '0') * 1000 + (__DATE__[8] - '0') * 100 + (__DATE__[9] - '0') * 10 + (__DATE__[10] - '0');
	constexpr unsigned int compileMonth = (__DATE__[0] == 'J') ? ((__DATE__[1] == 'a') ? 1 : ((__DATE__[2] == 'n') ? 6 : 7))    // Jan, Jun or Jul
		: (__DATE__[0] == 'F') ? 2                                                              // Feb
		: (__DATE__[0] == 'M') ? ((__DATE__[2] == 'r') ? 3 : 5)                                 // Mar or May
		: (__DATE__[0] == 'A') ? ((__DATE__[2] == 'p') ? 4 : 8)                                 // Apr or Aug
		: (__DATE__[0] == 'S') ? 9                                                              // Sep
		: (__DATE__[0] == 'O') ? 10                                                             // Oct
		: (__DATE__[0] == 'N') ? 11                                                             // Nov
		: (__DATE__[0] == 'D') ? 12                                                             // Dec
		: 0;
	constexpr unsigned int compileDay = (__DATE__[4] == ' ') ? (__DATE__[5] - '0') : (__DATE__[4] - '0') * 10 + (__DATE__[5] - '0');

	constexpr char IsoDate[] =
	{ compileYear / 1000 + '0', (compileYear % 1000) / 100 + '0', (compileYear % 100) / 10 + '0', compileYear % 10 + '0',
		'-',  compileMonth / 10 + '0', compileMonth % 10 + '0',
		'-',  compileDay / 10 + '0', compileDay % 10 + '0',
		0
	};

	MainLoop::getInstance().window = glfwCreateWindow(1920, 1080, (appName + std::string(" :: ") + BUILD_VERSION + " :: build " + IsoDate).c_str(), NULL, NULL);

#if FULLSCREEN_MODE
	setFullscreen(true, true);
#endif

	if (!MainLoop::getInstance().window)
	{
		glfwTerminate();
	}

	glfwMakeContextCurrent(MainLoop::getInstance().window);

	// NOTE: glfwswapinterval() needs to be executed after glfwmakecontextcurrent()
#if V_SYNC
	glfwSwapInterval(1);
#else
	glfwSwapInterval(0);
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

#ifdef _DEVELOP
void setupImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
	io.ConfigWindowsResizeFromEdges = true;

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1;
	style.FrameRounding = 1;
	style.WindowTitleAlign.x = 0.5f;
	style.WindowMenuButtonPosition = 1;		// Right side of menu
	style.WindowRounding = 0;

	ImGui_ImplGlfw_InitForOpenGL(MainLoop::getInstance().window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
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

	//pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT
	//	| physx::PxPairFlag::eDETECT_CCD_CONTACT
	//	| physx::PxPairFlag::eNOTIFY_TOUCH_CCD
	//	| physx::PxPairFlag::eNOTIFY_TOUCH_FOUND
	//	| physx::PxPairFlag::eNOTIFY_CONTACT_POINTS
	//	| physx::PxPairFlag::eCONTACT_EVENT_POSE;

	pairFlags = physx::PxPairFlag::eSOLVE_CONTACT
		| physx::PxPairFlag::eDETECT_DISCRETE_CONTACT;
		//| physx::PxPairFlag::eDETECT_CCD_CONTACT;		// NOTE: doesn't seem to work, so let's not waste compute resources on this.

	return physx::PxFilterFlags();  //physx::PxFilterFlag::eDEFAULT;
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

	const uint32_t numCores = std::thread::hardware_concurrency();
	gDispatcher = physx::PxDefaultCpuDispatcherCreate(numCores == 0 ? 1 : numCores);
	physx::PxSceneDesc sceneDesc(MainLoop::getInstance().physicsPhysics->getTolerancesScale());
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.gravity = physx::PxVec3(0, -98.1f, 0);
	sceneDesc.filterShader = contactReportFilterShader;  // physx::PxDefaultSimulationFilterShader; // contactReportFilterShader;
	sceneDesc.simulationEventCallback = &gContactReportCallback;
	sceneDesc.flags |= physx::PxSceneFlag::eENABLE_CCD;
	sceneDesc.ccdMaxPasses = 4;

	MainLoop::getInstance().physicsCooking = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, physx::PxCookingParams(tolerancesScale));
	if (!MainLoop::getInstance().physicsCooking)
		std::cerr << "FATAL ERROR::::PxCreateCooking failed!" << std::endl;

	MainLoop::getInstance().physicsScene = MainLoop::getInstance().physicsPhysics->createScene(sceneDesc);
	MainLoop::getInstance().defaultPhysicsMaterial = MainLoop::getInstance().physicsPhysics->createMaterial(0.5f, 0.5f, 1.0f);

#ifdef _DEVELOP
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
#ifdef _DEVELOP
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
			
#ifdef _DEVELOP
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
