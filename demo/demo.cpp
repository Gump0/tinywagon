#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <tinywagon/tinywagon.hpp>
#include <iostream>

#include "openglErrorReporting.hpp"

static void error_callback(int error, const char* description)
{
    std::cerr << "ERROR: " << description << std::endl;
}

int main(void)
{
    // boilerplate
    const int width { 1280 };
    const int height { 720 };
    
    glfwInit(); 
    glfwWindowHint(GLFW_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT);
#endif

    GLFWwindow* window = glfwCreateWindow(width, height, "tinywagon test window", nullptr,  nullptr);
    if (window == nullptr)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
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

    // glfwSwapInterval(1); // vsync.

    float lastFrameTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        float currentFrameTime = (float)glfwGetTime();
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0, 0, 0.5, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}