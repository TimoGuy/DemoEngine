#include "RenderManager.h"

#include <stb/stb_image.h>
#include <vector>
#include <string>
#include <cmath>
#include <glm/gtx/scalar_multiplication.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_impl_glfw.h"
#include "../../ImGui/imgui_impl_opengl3.h"

#include "../Camera.h"
#include "../RenderEngine.model/RenderEngine.model.animation/Animator.h"

#define SINGLE_BUFFERED_MODE 0
#if SINGLE_BUFFERED_MODE
#include <chrono>
#include <thread>
#endif


Camera camera;
void frameBufferSizeChangedCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	camera.width = width;
	camera.height = height;
}

RenderManager::RenderManager()
{
	modelPosition = glm::vec3(5.0f, 0.0f, 0.0f);
	modelEulerAngles = glm::vec3(90.0f, 0.0f, 0.0f);
	this->initialize();
}

RenderManager::~RenderManager()
{
	this->finalize();
}

const GLchar* RenderManager::readFile(const char* filename)
{
	FILE* infile;

	fopen_s(&infile, filename, "rb");
	if (!infile) {
		printf("Unable to open file %s\n", filename);
		return NULL;
	}

	fseek(infile, 0, SEEK_END);
	int len = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	GLchar* source = (GLchar*)malloc(sizeof(GLchar) * (len + 1));
	fread(source, 1, len, infile);
	fclose(infile);
	source[len] = 0;

	return (GLchar*)(source);
}

void RenderManager::initialize()
{
	createWindow("Test Window");
	setupViewPort();
	setupImGui();

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.1f, 0.2f, 1.0f);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	lightPosition = glm::vec3(7.0f, 5.0f, 3.0f);

	createProgram();
	createRect();
}

void RenderManager::setupViewPort()
{
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glfwSetFramebufferSizeCallback(window, frameBufferSizeChangedCallback);
	frameBufferSizeChangedCallback(nullptr, width, height);
}

void RenderManager::setupImGui()
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

GLuint texture, cubemapTexture;
void RenderManager::createRect()
{
	//
	// Create Quad
	//
	{
		float vertices[] = {
			-.5f,	0.5f,	-.1f,			0.0f, 1.0f,
			-.5f,	-.5f,	0.1f,			0.0f, 0.0f,
			0.5f,	0.5f,	-.1f,			1.0f, 1.0f,
			0.5f,	0.5f,	-.1f,			1.0f, 1.0f,
			-.5f,	-.5f,	0.1f,			0.0f, 0.0f,
			0.5f,	-.5f,	0.1f,			1.0f, 0.0f
		};

		GLuint indices[] = {
			0, 1, 2,
			3, 4, 5
		};

		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer((GLuint)1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	//
	// Create Texture
	//
	{
		int imgWidth, imgHeight, numColorChannels;
		stbi_set_flip_vertically_on_load(true);
		unsigned char* bytes = stbi_load("res/cool_img.png", &imgWidth, &imgHeight, &numColorChannels, STBI_default);

		//std::cout << imgWidth << "x" << imgHeight << " (" << numColorChannels << std::endl;

		if (bytes == NULL)
		{
			std::cout << stbi_failure_reason() << std::endl;
		}

		glGenTextures(1, &texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(bytes);
		glBindTexture(GL_TEXTURE_2D, 0);

		glUniform1i(
			glGetUniformLocation(this->program_id, "tex0"),
			0
		);
	}

	//
	// Create Cubemap for Skybox
	//
	{
		std::vector<std::string> textureFaces = {
			std::string("res/skybox/bluecloud_rt.jpg"),
			std::string("res/skybox/bluecloud_lf.jpg"),
			std::string("res/skybox/bluecloud_up.jpg"),
			std::string("res/skybox/bluecloud_dn.jpg"),
			std::string("res/skybox/bluecloud_ft.jpg"),
			std::string("res/skybox/bluecloud_bk.jpg")
		};

		glGenTextures(1, &cubemapTexture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

		int width, height, nrChannels;
		unsigned char* data;
		for (unsigned int i = 0; i < textureFaces.size(); i++)
		{
			data = stbi_load(textureFaces[i].c_str(), &width, &height, &nrChannels, STBI_default);
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		//glUniform1i(		// OPTIONAL
		//	glGetUniformLocation(this->skybox_program_id, "skyboxTex"),
		//	0
		//);
	}

	//
	// Create VAO, VBO for Skybox
	//
	{
		float skyboxVertices[] = {
			// positions          
			-1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			-1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f
		};

		glGenVertexArrays(1, &skyboxVAO);
		glGenBuffers(1, &skyboxVBO);

		glBindVertexArray(skyboxVAO);

		glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
}

void RenderManager::createProgram()
{
	int vShader, fShader;

	this->program_id = glCreateProgram();
	vShader = createShader(GL_VERTEX_SHADER, "vertex.vert");
	fShader = createShader(GL_FRAGMENT_SHADER, "fragment.frag");
	glAttachShader(this->program_id, vShader);
	glAttachShader(this->program_id, fShader);
	glLinkProgram(this->program_id);
	glDeleteShader(vShader);
	glDeleteShader(fShader);

	this->model_program_id = glCreateProgram();
	vShader = createShader(GL_VERTEX_SHADER, "model.vert");
	fShader = createShader(GL_FRAGMENT_SHADER, "model.frag");
	glAttachShader(this->model_program_id, vShader);
	glAttachShader(this->model_program_id, fShader);
	glLinkProgram(this->model_program_id);
	glDeleteShader(vShader);
	glDeleteShader(fShader);

	skybox_program_id = glCreateProgram();
	vShader = createShader(GL_VERTEX_SHADER, "skybox.vert");
	fShader = createShader(GL_FRAGMENT_SHADER, "skybox.frag");
	glAttachShader(skybox_program_id, vShader);
	glAttachShader(skybox_program_id, fShader);
	glLinkProgram(skybox_program_id);
	glDeleteShader(vShader);
	glDeleteShader(fShader);
}

int RenderManager::createShader(GLenum type, const char* fname)
{
	GLuint shader_id = glCreateShader(type);
	const char* filedata = readFile(fname);

	glShaderSource(shader_id, 1, &filedata, NULL);
	glCompileShader(shader_id);

	GLint compiled;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLsizei len;
		glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &len);

		GLchar* log = (GLchar*)malloc(sizeof(GLchar) * (len + 1));
		glGetShaderInfoLog(shader_id, len, &len, log);
		printf("Shader compilation failed: %s\n", log);
		free((GLchar*)log);
	}

	return shader_id;
}

int RenderManager::run(void)
{
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// TEMP: try model loading
	Model tryModel("res/player_lava.fbx");
	Animation runAnimation("res/player_lava.fbx", &tryModel);
	Animator animator(&runAnimation);

	float zFar = 2000.0f;

#if SINGLE_BUFFERED_MODE
	const float desiredFrameTime = 1000.0f / 80.0f;
	float startFrameTime = 0;
#endif

	float deltaTime;
	float lastFrame = glfwGetTime();

	while (!glfwWindowShouldClose(this->window))
	{
#if SINGLE_BUFFERED_MODE
		startFrameTime = glfwGetTime();
#endif

		glfwPollEvents();

		if (!io.WantCaptureMouse || ImGui::GetMouseCursor() == ImGuiMouseCursor_None)
			camera.Inputs(window);

		// Update Animation
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		animator.updateAnimation(deltaTime);

		//
		// ImGui Render Pass
		//
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			renderImGui();

			// Render
			ImGui::Render();
		}

		//
		// Render
		//
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Draw skybox
			glDepthMask(GL_FALSE);
			glUseProgram(skybox_program_id);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
			camera.Matrix(45.0f, 0.1f, zFar, this->skybox_program_id, "skyboxProjViewMatrix", true);
			glBindVertexArray(skyboxVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glDepthMask(GL_TRUE);

			// Draw quad
			glUseProgram(this->program_id);
			glBindTexture(GL_TEXTURE_2D, texture);
			camera.Matrix(45.0f, 0.1f, zFar, this->program_id, "cameraMatrix", false);		// Uniforms must be set after the shader program is set
			glBindVertexArray(this->vao);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

			// Draw the model!!!!
			glUseProgram(this->model_program_id);
			camera.Matrix(45.0f, 0.1f, zFar, this->model_program_id, "cameraMatrix", false);
			glm::mat4 modelMatrix =
				glm::translate(modelPosition)
				* glm::eulerAngleXYZ(glm::radians(modelEulerAngles.x), glm::radians(modelEulerAngles.y), glm::radians(modelEulerAngles.z))
				* glm::scale(
					glm::mat4(1.0f),
					glm::vec3(0.0001f)
				);
			glUniformMatrix4fv(
				glGetUniformLocation(this->model_program_id, "modelMatrix"),
				1,
				GL_FALSE,
				glm::value_ptr(modelMatrix)
			);
			glUniformMatrix3fv(
				glGetUniformLocation(this->model_program_id, "normalsModelMatrix"),
				1,
				GL_FALSE,
				glm::value_ptr(glm::mat3(glm::transpose(glm::inverse(modelMatrix))))
			);
			glUniform3fv(glGetUniformLocation(this->model_program_id, "lightPosition"), 1, &lightPosition[0]);
			glUniform3fv(glGetUniformLocation(this->model_program_id, "viewPosition"), 1, &camera.position[0]);
			glUniform3f(glGetUniformLocation(this->model_program_id, "lightColor"), 1.0f, 1.0f, 1.0f);
			auto transforms = animator.getFinalBoneMatrices();
			for (int i = 0; i < transforms.size(); i++)
				glUniformMatrix4fv(
					glGetUniformLocation(this->model_program_id, ("finalBoneMatrices[" + std::to_string(i) + "]").c_str()),
					1,
					GL_FALSE,
					glm::value_ptr(transforms[i])
				);
			tryModel.render(this->model_program_id);
		}


		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#if SINGLE_BUFFERED_MODE
		glFlush();
		std::this_thread::sleep_for(std::chrono::milliseconds((unsigned int)std::max(0.0, desiredFrameTime - (glfwGetTime() - startFrameTime))));
#else
		glfwSwapBuffers(this->window);
#endif
	}

	glfwTerminate();
	return 0;
}

void RenderManager::renderImGui()
{
	static bool showAnalyticsOverlay = true;
	static bool showScenePropterties = true;
	static float gizmoSize1to1 = 30.0f;
	static float clipZ;

	static bool renderWireframeMode = false;

	//
	// Menu Bar
	//
	{
		if (ImGui::BeginMainMenuBar())
		{
			//ImGui::begin
			if (ImGui::BeginMenu("File"))
			{
				//ShowExampleMenuFile();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
				if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
				ImGui::Separator();
				if (ImGui::MenuItem("Cut", "CTRL+X")) {}
				if (ImGui::MenuItem("Copy", "CTRL+C")) {}
				if (ImGui::MenuItem("Paste", "CTRL+V")) {}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Window"))
			{
				if (ImGui::MenuItem("Analytics Overlay", NULL, &showAnalyticsOverlay)) {}
				if (ImGui::MenuItem("Scene Properties", NULL, &showScenePropterties)) {}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Rendering"))
			{
				if (ImGui::MenuItem("Wireframe Mode", NULL, &renderWireframeMode))
				{
					glPolygonMode(GL_FRONT_AND_BACK, renderWireframeMode ? GL_LINE : GL_FILL);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
	}

	//
	// Easy Analytics Overlay
	//
	if (showAnalyticsOverlay)
	{
		// FPS counter
		static double prevTime = 0.0, crntTime = 0.0;
		static unsigned int counter = 0;
		static std::string fpsReportString;

		crntTime = glfwGetTime();
		double timeDiff = crntTime - prevTime;
		counter++;
		if (timeDiff >= 1.0 / 7.5)
		{
			std::string fps = std::to_string(1.0 / timeDiff * counter).substr(0, 5);
			std::string ms = std::to_string(timeDiff / counter * 1000).substr(0, 5);
			fpsReportString = "FPS: " + fps + " (" + ms + "ms)";
			prevTime = crntTime;
			counter = 0;
		}


		const float PAD = 10.0f;
		static int corner = 0;
		ImGuiIO& io = ImGui::GetIO();
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
		if (corner != -1)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
			ImVec2 work_size = viewport->WorkSize;
			ImVec2 window_pos, window_pos_pivot;
			window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
			window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
			window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
			window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
			ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
			window_flags |= ImGuiWindowFlags_NoMove;
		}
		ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
		if (ImGui::Begin("Example: Simple overlay", &showAnalyticsOverlay, window_flags))
		{
			ImGui::Text(fpsReportString.c_str());
			ImGui::Separator();
			if (ImGui::IsMousePosValid())
				ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
			else
				ImGui::Text("Mouse Position: <invalid>");
			/*if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Custom", NULL, corner == -1)) corner = -1;
				if (ImGui::MenuItem("Top-left", NULL, corner == 0)) corner = 0;
				if (ImGui::MenuItem("Top-right", NULL, corner == 1)) corner = 1;
				if (ImGui::MenuItem("Bottom-left", NULL, corner == 2)) corner = 2;
				if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
				if (showAnalyticsOverlay && ImGui::MenuItem("Close")) showOverlay = false;
				ImGui::EndPopup();
			}*/
		}
		ImGui::End();
	}

	//
	// Demo window
	//
	static bool show = true;
	if (show)
		ImGui::ShowDemoWindow(&show);

	//
	// Scene Properties window
	//
	if (showScenePropterties)
	{
		if (ImGui::Begin("Scene Properties", &showScenePropterties, ImGuiWindowFlags_AlwaysAutoResize))
		{
			static float lightPosVec[3] = { lightPosition.x, lightPosition.y, lightPosition.z };
			ImGui::DragFloat3("Light Position", lightPosVec);

			lightPosition.x = lightPosVec[0];
			lightPosition.y = lightPosVec[1];
			lightPosition.z = lightPosVec[2];
			ImGui::DragFloat("Gizmo Size one to one", &gizmoSize1to1);
			ImGui::Text("ClipZ: %.2f", clipZ);
			ImGui::Separator();

			ImGui::DragFloat3("Model Position", &modelPosition.x);
			ImGui::DragFloat3("Model Euler Angles", &modelEulerAngles.x);
		}
		ImGui::End();
	}

	//
	// Draw Light position
	//
	glm::vec3 lightPosOnScreen = camera.PositionToClipSpace(lightPosition);
	clipZ = lightPosOnScreen.z;


	float gizmoRadius = gizmoSize1to1;
	{
		float a = 0.0f;
		float b = gizmoSize1to1;

		float defaultHeight = std::tanf(glm::radians(45.0f)) * clipZ;
		float t = gizmoSize1to1 / defaultHeight;
		//std::cout << t << std::endl;
		gizmoRadius = t * (b - a) + a;
	}

	if (clipZ > 0.0f)
	{
		lightPosOnScreen /= clipZ;
		lightPosOnScreen.x = lightPosOnScreen.x * camera.width / 2 + camera.width / 2;
		lightPosOnScreen.y = -lightPosOnScreen.y * camera.height / 2 + camera.height / 2;
		//std::cout << lightPosOnScreen.x << ", " << lightPosOnScreen.y << ", " << lightPosOnScreen.z << std::endl;
		ImVec2 p_min = ImVec2(lightPosOnScreen.x - gizmoRadius, lightPosOnScreen.y + gizmoRadius);
		ImVec2 p_max = ImVec2(lightPosOnScreen.x + gizmoRadius, lightPosOnScreen.y - gizmoRadius);

		ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)texture, p_min, p_max);
	}
}

void RenderManager::createWindow(const char* windowName)
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

	// TODO: in the future may wanna use glfwGetWindowContentScale(window, &xscale, &yscale); for scaled desktop users (and everybody apple)
}

void RenderManager::finalize()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glDeleteTextures(1, &texture);
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteProgram(program_id);

	glfwDestroyWindow(window);
	glfwTerminate();
}