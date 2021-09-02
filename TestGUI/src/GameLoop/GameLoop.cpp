#include "GameLoop.h"

#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <thread>

#define SINGLE_BUFFERED_MODE 0
#if SINGLE_BUFFERED_MODE
#include <chrono>
#endif

#include "GameLoop.GameObject/GameObject.h"
#include "../RenderEngine/RenderEngine.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_glfw.h"
#include "../ImGui/imgui_impl_opengl3.h"
#include "../RenderEngine/Camera.h"


namespace GameLoop
{
	static bool isRunning;

	static GLFWwindow* window;

	static RenderManager* renderManager;
	static Camera camera;
	static std::vector<GameObject*> gameObjects;		// TODO: This gets read in from the scene

	static physx::PxFoundation* physicsFoundation;
	static physx::PxPhysics* physicsPhysics;
	static physx::PxCooking* physicsCooking;			// NOTE: Currently unused
	static physx::PxScene* physicsScene;


	void frameBufferSizeChangedCallback(GLFWwindow* window, int width, int height)
	{
		camera.width = width;
		camera.height = height;
	}


	void setupWindow(const char* windowName)
	{
		if (!glfwInit())
			glfwTerminate();

#if SINGLE_BUFFERED_MODE
		glfwWindowHint(GLFW_DOUBLEBUFFER, GL_FALSE);
		window = glfwCreateWindow(1920, 1080, windowName, NULL, NULL);
#else
		window = glfwCreateWindow(1920, 1080, windowName, NULL, NULL);
		glfwSwapInterval(1);
#endif

		if (!window)
		{
			glfwTerminate();
		}

		glfwMakeContextCurrent(window);
		gladLoadGL();
	}

	void setupViewport()
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glfwSetFramebufferSizeCallback(window, frameBufferSizeChangedCallback);
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

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 130");
	}



	/*physx::PxFilterFlags contactReportFilterShader(physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
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
}*/																												// NOTE: this appears to be important
	

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

	void setupPhysx()
	{
		ContactReportCallback gContactReportCallback;

		physx::PxDefaultErrorCallback gErrorCallback;
		physx::PxDefaultAllocator gAllocator;
		physx::PxDefaultCpuDispatcher* gDispatcher = NULL;
		physx::PxTolerancesScale tolerancesScale;

		physx::PxMaterial* gMaterial = NULL;
		physx::PxRigidStatic* gTriangleMeshActor = NULL;
		physx::PxRigidDynamic* gSphereActor = NULL;
		physx::PxPvd* gPvd = NULL;


		physx::PxRigidDynamic* body = NULL;







		physicsFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
		gPvd = PxCreatePvd(*physicsFoundation);
		physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
		gPvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

		tolerancesScale.length = 100;
		tolerancesScale.speed = 981;

		physicsPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *physicsFoundation, tolerancesScale, true, gPvd);

		const unsigned int numCores = std::thread::hardware_concurrency();
		gDispatcher = physx::PxDefaultCpuDispatcherCreate(numCores == 0 ? 0 : numCores - 1);
		physx::PxSceneDesc sceneDesc(physicsPhysics->getTolerancesScale());
		sceneDesc.cpuDispatcher = gDispatcher;
		sceneDesc.gravity = physx::PxVec3(0, -9.81f, 0);
		sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader; // contactReportFilterShader;
		sceneDesc.simulationEventCallback = &gContactReportCallback;
		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_CCD;
		sceneDesc.ccdMaxPasses = 4;



		physicsScene = physicsPhysics->createScene(sceneDesc);
		/*physx::PxPvdSceneClient* pvdClient = physicsScene->getScenePvdClient();
		if (pvdClient)
		{
			pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		}*/
		gMaterial = physicsPhysics->createMaterial(0.5f, 0.5f, 1.0f);



		physx::PxPvdSceneClient* pvdClient = physicsScene->getScenePvdClient();
		if (pvdClient)
		{
			pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}


		//
		// Init scene
		//
		physx::PxRigidStatic* groundPlane = physx::PxCreatePlane(*physicsPhysics, physx::PxPlane(0, 1, 0, 3.6f), *gMaterial);
		physicsScene->addActor(*groundPlane);

		float halfExtent = 1.0f;
		const int size = 50;
		physx::PxTransform t;
		physx::PxShape* shape = physicsPhysics->createShape(physx::PxBoxGeometry(halfExtent, halfExtent, halfExtent), *gMaterial);
		/*for (physx::PxU32 i = 0; i < size; i++)
		{
			for (physx::PxU32 j = 0; j < size - i; j++)
			{
				physx::PxTransform localTm(physx::PxVec3(physx::PxReal(j * 2) - physx::PxReal(size - i), physx::PxReal(i * 5 + 1), 0) * halfExtent);
				physx::PxRigidDynamic* body = physicsPhysics->createRigidDynamic(localTm);
				body->attachShape(*shape);

				physx::PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
				physicsScene->addActor(*body);
			}
		}*/

		physx::PxTransform localTm(physx::PxVec3(physx::PxReal(0), physx::PxReal(200), 0) * halfExtent);
		body = physicsPhysics->createRigidDynamic(localTm);
		body->attachShape(*shape);

		physx::PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
		physicsScene->addActor(*body);
		shape->release();
	}

	void configureGraphics()
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		glClearColor(0.0f, 0.1f, 0.2f, 1.0f);

		glDisable(GL_CULL_FACE);		// Turned off for the pre-processing part, then will be turned on for actual realtime rendering

		//
		// NOTE: activated here so that
		// during the pre-processing steps
		// then culling doesn't hinder it
		//
		/*glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);*/										// NOTE: skybox doesn't render with this on... needs some work.

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}



	void runPhysicsLoopAsync()
	{
		while (isRunning)
		{
			physicsScene->simulate(0.02f);		// NOTE: this is 1/50, so 50 fps
			physicsScene->fetchResults(true);
		}

		// Shutdown physx
		physicsPhysics->release();
		physicsFoundation->release();
	}


	// -------------------------------------------------------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------------------------------------------------------
	void initialize()
	{
		isRunning = true;

		setupWindow("Test Window");
		setupViewport();
		setupImGui();
		configureGraphics();

		renderManager = new RenderManager();
	}

	void run()
	{
		//
		// Start physics thread
		//
		std::thread physicsLoop(runPhysicsLoopAsync);
		physicsLoop.detach();


		//
		// Start graphics thread
		//
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

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

			// Update Animation
			if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
				animator.playAnimation(0, 12.0f);
			if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
				animator.playAnimation(1, 12.0f);
			if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
				animator.playAnimation(2, 12.0f);
			if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
				animator.playAnimation(3, 12.0f);
			if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
				animator.playAnimation(4, 12.0f);
			if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
				animator.playAnimation(5, 12.0f);

			float currentFrame = glfwGetTime();
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;
			animator.updateAnimation(deltaTime * deltaTimeMultiplier);

			//
			// ImGui Render Pass
			//
			renderManager->renderImGuiPass(camera);

			//
			// Render
			//
			renderManager->reportCamera(camera);
			renderManager->renderShadowPass(camera);
			renderManager->renderGeometryPass(camera);

			// ImGui buffer swap
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


#if SINGLE_BUFFERED_MODE
			glFlush();
			std::this_thread::sleep_for(std::chrono::milliseconds((unsigned int)std::max(0.0, desiredFrameTime - (glfwGetTime() - startFrameTime))));
#else
			glfwSwapBuffers(window);
#endif
		}

		// Let physics thread know to shut down now
		isRunning = false;
	}

	void destroy()
	{
		delete renderManager;

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(window);
		glfwTerminate();
	}
}


namespace GameLoopUtils
{
	glm::mat4 physxMat44ToGlmMat4(physx::PxMat44 mat4, glm::mat4 newMat4)
	{
		newMat4[0][0] = mat4[0][0];
		newMat4[0][1] = mat4[0][1];
		newMat4[0][2] = mat4[0][2];
		newMat4[0][3] = mat4[0][3];

		newMat4[1][0] = mat4[1][0];
		newMat4[1][1] = mat4[1][1];
		newMat4[1][2] = mat4[1][2];
		newMat4[1][3] = mat4[1][3];

		newMat4[2][0] = mat4[2][0];
		newMat4[2][1] = mat4[2][1];
		newMat4[2][2] = mat4[2][2];
		newMat4[2][3] = mat4[2][3];

		newMat4[3][0] = mat4[3][0];
		newMat4[3][1] = mat4[3][1];
		newMat4[3][2] = mat4[3][2];
		newMat4[3][3] = mat4[3][3];

		return newMat4;
	}
}
