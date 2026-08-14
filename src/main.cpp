#define GLFW_INCLUDE_NONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Shader.hpp"

#include <cstddef>
#include <exception>
#include <iostream>

constexpr int WINDOW_WIDTH = 1000;
constexpr int WINDOW_HEIGHT = 700;


//Purpose: 
// Check keyboard
// Clear old picture
// Draw new picture
// Show it on screen
// Repeat
struct Vertex {
    float position[3];
    float color[3];
};

void framebufferSizeCallback(
    GLFWwindow*,
    int width,
    int height
) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main()
{
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Failed to initialize GLFW.\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );

    GLFWwindow* window = glfwCreateWindow(
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        "Electron Field Simulator",
        nullptr,
        nullptr
    );

    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glfwSetFramebufferSizeCallback(
        window,
        framebufferSizeCallback
    );

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);

    //Dark blue background.
    glClearColor(0.02f, 0.03f, 0.08f, 1.0f);

    glfwSwapInterval(1);

    int exitCode = 0;

    try {
        Shader shader(
            "shaders/basic.vert",
            "shaders/basic.frag"
        );

        const Vertex vertices[] = {
            // Position                  // Colour
            {{-0.6f, -0.5f, 0.0f},      {1.0f, 0.0f, 0.0f}}, // Red
            {{ 0.6f, -0.5f, 0.0f},      {0.0f, 1.0f, 0.0f}}, // Green
            {{ 0.0f,  0.6f, 0.0f},      {0.0f, 0.0f, 1.0f}}  // Blue
        };

        GLuint vertexArrayObject = 0;
        GLuint vertexBufferObject = 0;

        glGenVertexArrays(1, &vertexArrayObject);
        glGenBuffers(1, &vertexBufferObject);

        glBindVertexArray(vertexArrayObject);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);

        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(vertices),
            vertices,
            GL_STATIC_DRAW
        );

        //Explain where position is stored in each Vertex.
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            static_cast<GLsizei>(sizeof(Vertex)),
            reinterpret_cast<void*>(
                offsetof(Vertex, position)
            )
        );

        glEnableVertexAttribArray(0);

        //Explain where colour is stored in each Vertex.
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            static_cast<GLsizei>(sizeof(Vertex)),
            reinterpret_cast<void*>(
                offsetof(Vertex, color)
            )
        );

        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        while (
            glfwWindowShouldClose(window) == GLFW_FALSE
        ) {
            processInput(window);

            glClear(
                GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT
            );

            shader.use();
            glBindVertexArray(vertexArrayObject);

            glDrawArrays(GL_TRIANGLES, 0, 3);

            glBindVertexArray(0);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        glDeleteBuffers(1, &vertexBufferObject);
        glDeleteVertexArrays(1, &vertexArrayObject);
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        exitCode = 1;
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return exitCode;
}

// GLFW creates the window
//         ↓
// GLEW loads the OpenGL functions
//         ↓
// GLM calculates positions and transformations
//         ↓
// OpenGL sends the drawing instructions to the GPU
//         ↓
// GLFW displays the completed frame