#include "RenderManager.h"

	RenderManager::RenderManager()
	{
		this->initialize();
		this->run();
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

		createProgram();
		createRect();
	}

	void frameBufferSizeChangedCallback(GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void RenderManager::setupViewPort()
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glfwSetFramebufferSizeCallback(window, frameBufferSizeChangedCallback);
		glViewport(0, 0, width, height);
	}

	void RenderManager::createRect()
	{
		float vertices[] = {
			-0.5, 0.5, 0,
			-0.5, -0.5, 0,
			0.5, 0.5, 0,
			0.5, 0.5, 0,
			-0.5, -0.5, 0,
			0.5, -0.5, 0
		};

		vbo = 0;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);

		vao = 0;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
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
			glClear(GL_COLOR_BUFFER_BIT);
			glUseProgram(this->program_id);
			glBindVertexArray(this->vao);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glfwPollEvents();
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
	}