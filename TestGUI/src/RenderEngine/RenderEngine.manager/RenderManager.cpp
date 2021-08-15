#include "RenderManager.h"

#include <stb/stb_image.h>
#include <vector>
#include <string>
#include <glm/gtx/scalar_multiplication.hpp>

#include "../Camera.h"
#include "../RenderEngine.model/Model.h"


Camera camera;
void frameBufferSizeChangedCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	camera.width = width;
	camera.height = height;
}

RenderManager::RenderManager()
{
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

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.1f, 0.2f, 1.0f);
	/*glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);*/

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

		std::cout << imgWidth << "x" << imgHeight << " (" << numColorChannels << std::endl;

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
	// TEMP: try model loading
	Model tryModel("res/player_slime.fbx");

	float zFar = 2000.0f;

	while (!glfwWindowShouldClose(this->window))
	{
		glfwPollEvents();
		camera.Inputs(window);

		//
		// Render
		//
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
			glm::translate(glm::vec3(5.0f, 0.0f, 0.0f))
			* glm::scale(
				glm::mat4(1.0f),
				glm::vec3(0.01f)
			);
		glUniformMatrix4fv(
			glGetUniformLocation(this->model_program_id, "modelMatrix"),
			1,
			GL_FALSE,
			glm::value_ptr(modelMatrix)
		);

		glUniform3f(glGetUniformLocation(this->model_program_id, "lightPosition"), 500.0f, 500.0f, 200.0f);
		glUniform3fv(glGetUniformLocation(this->model_program_id, "viewPosition"), 1, &camera.position[0]);
		glUniform3f(glGetUniformLocation(this->model_program_id, "lightColor"), 1.0f, 1.0f, 1.0f);

		tryModel.render(this->model_program_id);

		glfwSwapBuffers(this->window);
	}

	glfwTerminate();
	return 0;
}

void RenderManager::createWindow(const char* windowName)
{
	if (!glfwInit())
		glfwTerminate();

	window = glfwCreateWindow(1280, 720, windowName, NULL, NULL);

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
	glDeleteTextures(1, &texture);
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteProgram(program_id);

	glfwDestroyWindow(window);
	glfwTerminate();
}