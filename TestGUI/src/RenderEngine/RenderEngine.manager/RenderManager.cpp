#include "RenderManager.h"

	RenderManager::RenderManager()
	{
		this->initialize();
		//this->run();
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
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		createProgram();
		createRect();
	}

	void RenderManager::createRect()
	{

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
			for (int i = 0; i < this->renderList.size(); i++)
				this->renderList.at(i).render();
			//glDrawArrays(GL_TRIANGLES, 0, 6); // TODO change size from 6 to however many verts need to be drawn, or glDrawElements??

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
	}

	void RenderManager::finalize()
	{
	}

	std::vector<RenderableObject>* RenderManager::getRenderList()
	{
		return &this->renderList;
	}