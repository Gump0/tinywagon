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

	GLFWwindow *window = glfwCreateWindow(640, 480, "tinywagon example", NULL, NULL);
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
	// -----------------------------------------
	tw::init();

	tw::Renderer2D renderer;
	renderer.create();


	tw::Shader shader;
	shader.createDefaultShader();

	tw::Texture texture1(RESOURCES_PATH "grass.png");
	texture1.bind();

	tw::Texture texture2;
	texture2.loadFromFileWithPixelPadding(RESOURCES_PATH "structuredArt.png", 3, 2);
	tw::TextureAtlasPadding textureAtlas(3, 2, texture2.getSize().x, texture2.getSize().y);

	// Example of loading font.
	// but for this example we use the default font
	// tw::Font font(RESOURCES_PATH "font.ttf");

	std::cout << "loading texture from: " << RESOURCES_PATH << "grass.png" << std::endl;
	std::cout << "loading texture from: " << RESOURCES_PATH << "strucuredArt.png" << std::endl;

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

		// render triangles with normalized coordinates.
		renderer.renderTriangleFromNormalizedPositions(
			{-0.5, -0.5, 0, 1}, {0.5, -0.5, 0, 1}, {0.0, 0.5, 0, 1}, { }, {0, 0, 1, 1}, WHITE);

		renderer.renderTriangleFromNormalizedPositions(
			{-0.1, -0.1, 0, 1}, {0.1, -0.1, 0, 1}, {0.0, 0.5, 0, 1}, { }, {0, 0, 1, 1}, PURPLE);

		// render triangle based off world coords
		renderer.renderTriangle({150, 150, 0, 0}, {-150, -150, 0, 0}, {0, 0, 0, 0}, texture1, {0, 0, 1, 1}, PINK);
		
		// render single square texture example.
		renderer.renderTriangle({150, 150, 0, 1}, {-150, 150, 0, 1}, {0, -150, 0, 1}, texture1, {0, 0, 1, 1}, PINK);

		// render textures from texture atlas example
		renderer.renderRect({200,200, 100, 100}, texture2, {1,1,1,1}, textureAtlas.get(0, 0));
		renderer.renderRect({300,200, 100, 100}, texture2, {1,1,1,1}, textureAtlas.get(1, 1, true));

		renderer.renderRectFromNormalizedPostions({-0.3, -0.3, 0.1, 0.1}, texture1);

		// render entire font texture
		// renderer.renderRect({400, 400, 1000, 1000} , { }, {0.3, 0.1, 0.1, 1});
		// renderer.renderRect({400, 400, 1000, 1000}, font.texture, RED, {0, 1, 1, 0});

		// render silly text boxes
		// auto letters = tw::computeTextLayout("Test\nTest2\nTes t3", font, 64, 1, 1, true);

		// for (auto &l : letters.letters)
		// {
		// 	renderer.renderRect(l.rectangle, font.texture, {1,1,1,1}, l.textureCoords);
		// }

		// more 'official way' to render text.
		auto testText = renderer.renderText({0,0}, "Foo\nBar\nTes t 24");

		// render text cetner point for display purpose
		renderer.renderRect({-2,-2,4,4}, {}, {1,0,0,1});

		// render text dimensions as a small box.
		renderer.renderRect({testText.min, testText.getSize()}, {}, {0, 1, 1, 0.2});

		renderer.flush();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	tw::cleanup();

	glfwDestroyWindow(window);

	glfwTerminate();

	return 0;
}