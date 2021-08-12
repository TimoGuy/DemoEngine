#include "RenderManager.h"
#include <stb/stb_image.h>
#include "../Camera.h"

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

GLuint texture;
void RenderManager::createRect()
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

	//
	// Create Texture
	//
	int imgWidth, imgHeight, numColorChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* bytes = stbi_load("cool_img.png", &imgWidth, &imgHeight, &numColorChannels, STBI_default);

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

void RenderManager::createProgram()
{
	this->program_id = glCreateProgram();
	this->vert_shader = createShader(GL_VERTEX_SHADER, "vertex.vert");
	this->frag_shader = createShader(GL_FRAGMENT_SHADER, "fragment.frag");
	glAttachShader(this->program_id, this->vert_shader);
	glAttachShader(this->program_id, this->frag_shader);
	glLinkProgram(this->program_id);
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
	while (!glfwWindowShouldClose(this->window))
	{
		glfwPollEvents();
		camera.Inputs(window);
		camera.Matrix(45.0f, 0.1f, 100.0f, this->program_id, "cameraMatrix");

		//
		// Render
		//
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(this->program_id);
		glBindTexture(GL_TEXTURE_2D, texture);

		glBindVertexArray(this->vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

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