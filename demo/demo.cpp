#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <tinywagon/tinywagon.hpp>
#include <openglErrorReporting.hpp>


static void error_callback(int error, const char *description)
{
	std::cout << "Error: " << description << "\n";
}

int main(void)
{
	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		return EXIT_FAILURE;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif

	GLFWwindow *window = glfwCreateWindow(640, 480, "Pixy example", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		glfwTerminate();
		return EXIT_FAILURE;
	}

	enableReportGlErrors();

	//glfwSwapInterval(1); //vsync

	//---------------- TRIANGLE ----------------
	//position, color, uv
	float vertices[] =
	{
		-0.5f, -0.5f,  1,1,1,1,  0.f, 0.f, 
		 0.5f, -0.5f,  1,1,1,1,  1.f, 0.f,
		 0.0f,  0.5f,  1,1,1,1,  0.f, 1.f,
	};

	GLuint vao = 0;
	GLuint vbo = 0;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 2));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 6));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	// ------------------------------------------
	tw::init();

	tw::Renderer2D renderer;
	renderer.create();

	tw::Shader shader;
	shader.createDefaultShader();

	tw::Texture texture(RESOURCES_PATH "grass.png");
	texture.bind();
	std::cout << "loading texture from: " << RESOURCES_PATH << "grass.png" << std::endl;

	float lastFrameTime = (float)glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
		float currentFrameTime = (float)glfwGetTime();
		float deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);

		renderer.updateWindowMetrics(width, height);

		glViewport(0, 0, width, height);
		glClearColor(0, 0, 0.5, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glm::vec2 movement = { };
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { movement.y -= 1.0f; }
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { movement.y += 1.0f; }
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { movement.x -= 1.0f; }
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { movement.x += 1.0f; }

		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) { renderer.camera.zoom -= 0.01f; }
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) { renderer.camera.zoom += 0.01f; }

		static glm::vec2 playerPos = { };

		if (glm::length(movement) > 0.0f)
		{
			float speed = 250;
			playerPos += glm::normalize(movement) * speed * deltaTime;
		}

		renderer.camera.follow(playerPos, width, height, 100, 0, 100);

		renderer.renderTriangleFromNormalizedPositions(
			{-0.5, -0.5, 0, 1}, {0.5, -0.5, 0, 1}, {0.0, 0.5, 0, 1}, { }, {0, 0, 1, 1}, {1, 1, 1, 1});

		renderer.renderTriangleFromNormalizedPositions(
			{-0.1, -0.1, 0, 1}, {0.1, -0.1, 0, 1}, {0.0, 0.5, 0, 1}, { }, {0, 0, 1, 1}, PURPLE);

		renderer.renderRect({ 50, 50, 100, 100}, texture, {1, 1, 1, 1}, {0, 0, 1, 1}, 0);

		renderer.flush();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	tw::cleanup();

	glfwDestroyWindow(window);

	glfwTerminate();

	return 0;
}