#define GLFW_INCLUDE_NONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Shader.hpp"
#include "Sphere.hpp"

#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <vector>

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

// Creates pairs of vertices. Each pair becomes one line with GL_LINES.
std::vector<Vertex> createGridVertices()
{
    std::vector<Vertex> vertices;

    constexpr int GRID_LINE_COUNT = 12;
    constexpr float GRID_SPACING = 0.25f;
    constexpr float GRID_Y = -0.8f;
    constexpr float GRID_HALF_SIZE =
        GRID_LINE_COUNT * GRID_SPACING;

    for (int index = -GRID_LINE_COUNT;
         index <= GRID_LINE_COUNT;
         ++index) {
        if (index == 0) {
            continue;
        }

        const float position = index * GRID_SPACING;

        // Two grid lines: one follows Z, the other follows X.
        vertices.push_back({
            {position, GRID_Y, -GRID_HALF_SIZE},
            {0.18f, 0.38f, 0.50f}
        });
        vertices.push_back({
            {position, GRID_Y, GRID_HALF_SIZE},
            {0.18f, 0.38f, 0.50f}
        });
        vertices.push_back({
            {-GRID_HALF_SIZE, GRID_Y, position},
            {0.18f, 0.38f, 0.50f}
        });
        vertices.push_back({
            {GRID_HALF_SIZE, GRID_Y, position},
            {0.18f, 0.38f, 0.50f}
        });
    }

    // Coloured reference axes: X is red, Y is green, Z is blue.
    vertices.push_back({
        {-GRID_HALF_SIZE, GRID_Y, 0.0f},
        {1.0f, 0.18f, 0.18f}
    });
    vertices.push_back({
        {GRID_HALF_SIZE, GRID_Y, 0.0f},
        {1.0f, 0.18f, 0.18f}
    });
    vertices.push_back({
        {0.0f, GRID_Y, 0.0f},
        {0.20f, 1.0f, 0.25f}
    });
    vertices.push_back({
        {0.0f, 1.8f, 0.0f},
        {0.20f, 1.0f, 0.25f}
    });
    vertices.push_back({
        {0.0f, GRID_Y, -GRID_HALF_SIZE},
        {0.25f, 0.45f, 1.0f}
    });
    vertices.push_back({
        {0.0f, GRID_Y, GRID_HALF_SIZE},
        {0.25f, 0.45f, 1.0f}
    });

    return vertices;
}

// Stores the camera state that changes when the user moves the mouse.
struct OrbitCamera {
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);

    float yawDegrees = 32.0f;
    float pitchDegrees = 20.0f;
    float distance = 5.7f;

    bool isDragging = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

    glm::mat4 viewMatrix() const
    {
        const float yaw = glm::radians(yawDegrees);
        const float pitch = glm::radians(pitchDegrees);

        const glm::vec3 position =
            target + glm::vec3(
                distance * std::cos(pitch) * std::sin(yaw),
                distance * std::sin(pitch),
                distance * std::cos(pitch) * std::cos(yaw)
            );

        return glm::lookAt(
            position,
            target,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }
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

void mouseButtonCallback(
    GLFWwindow* window,
    int button,
    int action,
    int
) {
    auto* camera = static_cast<OrbitCamera*>(
        glfwGetWindowUserPointer(window)
    );

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        camera->isDragging = (action == GLFW_PRESS);

        glfwGetCursorPos(
            window,
            &camera->lastMouseX,
            &camera->lastMouseY
        );
    }
}

void cursorPositionCallback(
    GLFWwindow* window,
    double mouseX,
    double mouseY
) {
    auto* camera = static_cast<OrbitCamera*>(
        glfwGetWindowUserPointer(window)
    );

    if (!camera->isDragging) {
        return;
    }

    const float changeX = static_cast<float>(
        mouseX - camera->lastMouseX
    );
    const float changeY = static_cast<float>(
        camera->lastMouseY - mouseY
    );

    camera->lastMouseX = mouseX;
    camera->lastMouseY = mouseY;

    constexpr float MOUSE_SENSITIVITY = 0.25f;

    camera->yawDegrees += changeX * MOUSE_SENSITIVITY;
    camera->pitchDegrees += changeY * MOUSE_SENSITIVITY;

    // Avoid flipping over when looking directly up or down.
    if (camera->pitchDegrees > 89.0f) {
        camera->pitchDegrees = 89.0f;
    }
    if (camera->pitchDegrees < -89.0f) {
        camera->pitchDegrees = -89.0f;
    }
}

void scrollCallback(
    GLFWwindow* window,
    double,
    double scrollAmount
) {
    auto* camera = static_cast<OrbitCamera*>(
        glfwGetWindowUserPointer(window)
    );

    camera->distance -=
        static_cast<float>(scrollAmount) * 0.4f;

    if (camera->distance < 1.5f) {
        camera->distance = 1.5f;
    }
    if (camera->distance > 20.0f) {
        camera->distance = 20.0f;
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

    OrbitCamera camera;

    // GLFW passes the same camera pointer to each mouse callback.
    glfwSetWindowUserPointer(window, &camera);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetScrollCallback(window, scrollCallback);

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

        // The temporary triangle is now a small, round electron.
        Sphere electron(0.28f, 32, 20);

        const std::vector<Vertex> gridVertices =
            createGridVertices();

        GLuint gridVertexArrayObject = 0;
        GLuint gridVertexBufferObject = 0;

        glGenVertexArrays(1, &gridVertexArrayObject);
        glGenBuffers(1, &gridVertexBufferObject);

        glBindVertexArray(gridVertexArrayObject);
        glBindBuffer(GL_ARRAY_BUFFER, gridVertexBufferObject);

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                gridVertices.size() * sizeof(Vertex)
            ),
            gridVertices.data(),
            GL_STATIC_DRAW
        );

        // The grid uses the same position-and-colour Vertex layout.
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

        // Give the 3D scene a perspective: distant objects look smaller.
        const glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(WINDOW_WIDTH) /
                static_cast<float>(WINDOW_HEIGHT),
            0.1f,
            100.0f
        );

        while (
            glfwWindowShouldClose(window) == GLFW_FALSE
        ) {
            processInput(window);

            glClear(
                GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT
            );

            shader.use();

            //Build a fresh view matrix from the latest mouse input.
            const glm::mat4 view = camera.viewMatrix();
            shader.setMat4("view", view);
            int framebufferWidth = 0;
            int framebufferHeight = 0;

            glfwGetFramebufferSize(
                window,
                &framebufferWidth,
                &framebufferHeight
            );

            // This updates the camera shape to match the current window shape.
            const float aspectRatio =
                static_cast<float>(framebufferWidth) /
                static_cast<float>(framebufferHeight);

            const glm::mat4 projection = glm::perspective(
                glm::radians(45.0f),
                aspectRatio,
                0.1f,
                100.0f
            );
            shader.setMat4("projection", projection);

            //Draw the fixed grid before the electron.
            const glm::mat4 gridModel(1.0f);
            shader.setMat4("model", gridModel);

            glBindVertexArray(gridVertexArrayObject);
            glDrawArrays(
                GL_LINES,
                0,
                static_cast<GLsizei>(gridVertices.size())
            );
            glBindVertexArray(0);

            // Its position will come from physics in the next step.
            const glm::mat4 electronModel(1.0f);
            shader.setMat4("model", electronModel);
            electron.draw();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        glDeleteBuffers(1, &gridVertexBufferObject);
        glDeleteVertexArrays(1, &gridVertexArrayObject);
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
