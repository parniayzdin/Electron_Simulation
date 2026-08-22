#define GLFW_INCLUDE_NONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "Shader.hpp"
#include "Sphere.hpp"
#include "Particle.hpp"
#include "ElectromagneticField.hpp"
#include "Physics.hpp"

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

//Creates pairs of vertices. Each pair becomes one line with GL_LINES.
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

        //Two grid lines: one follows Z, the other follows X.
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

    //Coloured reference axes: X is red, Y is green, Z is blue.
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

//Adds one coloured line to a list that OpenGL will draw with GL_LINES.
void addLine(
    std::vector<Vertex>& vertices,
    const glm::vec3& start,
    const glm::vec3& end,
    const glm::vec3& color
) {
    vertices.push_back({
        {start.x, start.y, start.z},
        {color.r, color.g, color.b}
    });
    vertices.push_back({
        {end.x, end.y, end.z},
        {color.r, color.g, color.b}
    });
}

//Adds a clean headless field line to keep the dense field easy to read.
void addFieldLine(
    std::vector<Vertex>& vertices,
    const glm::vec3& start,
    const glm::vec3& direction,
    const glm::vec3& color
) {
    const glm::vec3 end = start + direction;
    addLine(vertices, start, end, color);
}

//Creates cyan magnetic-field arrows spread through the 3D scene.
std::vector<Vertex> createFieldVertices(
    const ElectromagneticField& field,
    float flowOffset
) {
    std::vector<Vertex> vertices;

    const glm::vec3 magneticUnitDirection =
        glm::normalize(field.magnetic);
    const glm::vec3 magneticDirection =
        magneticUnitDirection * 0.38f;

    const glm::vec3 magneticColor(0.28f, 0.90f, 1.0f);

    for (int y = 0; y < 3; ++y) {
        for (int x = -3; x <= 3; ++x) {
            for (int z = -3; z <= 3; ++z) {
                const glm::vec3 base(
                    static_cast<float>(x) * 0.80f,
                    -0.55f + static_cast<float>(y) * 0.72f,
                    static_cast<float>(z) * 0.80f
                );

                addFieldLine(
                    vertices,
                    base + magneticUnitDirection * flowOffset,
                    magneticDirection,
                    magneticColor
                );
            }
        }
    }

    return vertices;
}

//Keeps a short history so each electron leaves a visible path behind it.
void recordTrailPoint(Particle& particle)
{
    constexpr std::size_t MAX_TRAIL_POINTS = 180;
    constexpr float MINIMUM_TRAIL_DISTANCE = 0.015f;

    if (!particle.trail.empty()) {
        const float distanceSinceLastPoint = glm::length(
            particle.position - particle.trail.back()
        );

        if (distanceSinceLastPoint < MINIMUM_TRAIL_DISTANCE) {
            return;
        }
    }

    particle.trail.push_back(particle.position);

    if (particle.trail.size() > MAX_TRAIL_POINTS) {
        particle.trail.erase(particle.trail.begin());
    }
}

//Turns saved trail positions into coloured vertices for OpenGL.
std::vector<Vertex> createTrailVertices(const Particle& particle)
{
    std::vector<Vertex> vertices;
    vertices.reserve(particle.trail.size());

    for (const glm::vec3& position : particle.trail) {
        vertices.push_back({
            {position.x, position.y, position.z},
            {particle.color.r, particle.color.g, particle.color.b}
        });
    }

    return vertices;
}

//Stores the camera state that changes when the user moves the mouse.
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

void keepParticleInView(Particle& particle)
{
    constexpr float SIDE_BOUNDARY = 2.5f;
    constexpr float FLOOR = -0.45f;
    constexpr float CEILING = 1.5f;

    //Bounce on invisible walls so test electrons stay in the scene.
    if (particle.position.x > SIDE_BOUNDARY) {
        particle.position.x = SIDE_BOUNDARY;
        particle.velocity.x = -std::abs(particle.velocity.x);
    }
    if (particle.position.x < -SIDE_BOUNDARY) {
        particle.position.x = -SIDE_BOUNDARY;
        particle.velocity.x = std::abs(particle.velocity.x);
    }
    if (particle.position.z > SIDE_BOUNDARY) {
        particle.position.z = SIDE_BOUNDARY;
        particle.velocity.z = -std::abs(particle.velocity.z);
    }
    if (particle.position.z < -SIDE_BOUNDARY) {
        particle.position.z = -SIDE_BOUNDARY;
        particle.velocity.z = std::abs(particle.velocity.z);
    }
    if (particle.position.y > CEILING) {
        particle.position.y = CEILING;
        particle.velocity.y = -std::abs(particle.velocity.y);
    }
    if (particle.position.y < FLOOR) {
        particle.position.y = FLOOR;
        particle.velocity.y = std::abs(particle.velocity.y);
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

    //GLFW passes the same camera pointer to each mouse callback.
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

    //Dear ImGui draws the information panel on top of the OpenGL scene.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    int exitCode = 0;

    try {
        Shader shader(
            "shaders/basic.vert",
            "shaders/basic.frag"
        );

        // The temporary triangle is now a small, round electron.
        Sphere electron(0.28f, 32, 20);

        //Each particle has its own position and velocity.
        std::vector<Particle> electrons = {
            {
                glm::vec3(-0.9f, 0.3f, 0.2f),
                glm::vec3(0.5f, 0.1f, 0.2f),
                glm::vec3(0.20f, 0.65f, 1.0f),
                -1.0f,
                1.0f
            },
            {
                glm::vec3(0.6f, 0.6f, -0.4f),
                glm::vec3(-0.3f, -0.2f, 0.4f),
                glm::vec3(0.82f, 0.25f, 1.0f),
                -1.0f,
                1.0f
            },
            {
                glm::vec3(0.2f, -0.1f, 0.8f),
                glm::vec3(0.2f, 0.3f, -0.5f),
                glm::vec3(1.0f, 0.50f, 0.12f),
                -1.0f,
                1.0f
            }
        };

        //All electrons see the same uniform electric and magnetic fields.
        const ElectromagneticField field;

        const std::vector<Vertex> gridVertices =
            createGridVertices();

        const std::vector<Vertex> fieldVertices =
            createFieldVertices(field, 0.0f);

        GLuint gridVertexArrayObject = 0;
        GLuint gridVertexBufferObject = 0;
        GLuint fieldVertexArrayObject = 0;
        GLuint fieldVertexBufferObject = 0;
        std::vector<GLuint> trailVertexArrayObjects(
            electrons.size(),
            0
        );
        std::vector<GLuint> trailVertexBufferObjects(
            electrons.size(),
            0
        );

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

        //Each electron gets one small GPU buffer for its changing trail.
        glGenVertexArrays(
            static_cast<GLsizei>(trailVertexArrayObjects.size()),
            trailVertexArrayObjects.data()
        );
        glGenBuffers(
            static_cast<GLsizei>(trailVertexBufferObjects.size()),
            trailVertexBufferObjects.data()
        );

        for (std::size_t index = 0;
             index < electrons.size();
             ++index) {
            glBindVertexArray(trailVertexArrayObjects[index]);
            glBindBuffer(
                GL_ARRAY_BUFFER,
                trailVertexBufferObjects[index]
            );

            //This buffer will receive new trail points every frame.
            glBufferData(
                GL_ARRAY_BUFFER,
                0,
                nullptr,
                GL_DYNAMIC_DRAW
            );

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
        }
        glBindVertexArray(0);

        //The field arrows use the same Vertex structure as the grid.
        glGenVertexArrays(1, &fieldVertexArrayObject);
        glGenBuffers(1, &fieldVertexBufferObject);

        glBindVertexArray(fieldVertexArrayObject);
        glBindBuffer(GL_ARRAY_BUFFER, fieldVertexBufferObject);

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                fieldVertices.size() * sizeof(Vertex)
            ),
            fieldVertices.data(),
            GL_STATIC_DRAW
        );

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

        //This starts the timer after GLFW has initialized.
        float previousTime = static_cast<float>(glfwGetTime());

        while (
            glfwWindowShouldClose(window) == GLFW_FALSE
        ) {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            processInput(window);
            const float currentTime =
                static_cast<float>(glfwGetTime());

            const float deltaTime =
                currentTime - previousTime;

            //The arrows repeat every grid space, making the field appear to flow.
            const float fieldFlowOffset = std::fmod(
                currentTime * 0.35f,
                0.80f
            );

            const std::vector<Vertex> movingFieldVertices =
                createFieldVertices(field, fieldFlowOffset);

            previousTime = currentTime;

            //Move every electron using position = position + velocity × time.
            //Keep a very long paused frame from causing one huge physics jump.
            const float physicsTimeStep =
                deltaTime > 0.02f ? 0.02f : deltaTime;

            for (Particle& particle : electrons) {
                advanceParticleWithBoris(
                    particle,
                    field,
                    physicsTimeStep
                );
                keepParticleInView(particle);
                recordTrailPoint(particle);
            }

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
            shader.setFloat("particleColorWeight", 0.0f);
            shader.setFloat("brightness", 1.0f);

            glBindVertexArray(gridVertexArrayObject);
            glDrawArrays(
                GL_LINES,
                0,
                static_cast<GLsizei>(gridVertices.size())
            );
            glBindVertexArray(0);

            //Draw the arrows that show the uniform field directions.
            shader.setFloat("brightness", 1.15f);
            glBindBuffer(GL_ARRAY_BUFFER, fieldVertexBufferObject);
            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(
                    movingFieldVertices.size() * sizeof(Vertex)
                ),
                movingFieldVertices.data(),
                GL_DYNAMIC_DRAW
            );
            glBindVertexArray(fieldVertexArrayObject);
            glDrawArrays(
                GL_LINES,
                0,
                static_cast<GLsizei>(movingFieldVertices.size())
            );
            glBindVertexArray(0);

            //Upload and draw the recent path behind each moving electron.
            shader.setFloat("brightness", 0.85f);
            for (std::size_t index = 0;
                 index < electrons.size();
                 ++index) {
                const std::vector<Vertex> trailVertices =
                    createTrailVertices(electrons[index]);

                glBindBuffer(
                    GL_ARRAY_BUFFER,
                    trailVertexBufferObjects[index]
                );
                glBufferData(
                    GL_ARRAY_BUFFER,
                    static_cast<GLsizeiptr>(
                        trailVertices.size() * sizeof(Vertex)
                    ),
                    trailVertices.data(),
                    GL_DYNAMIC_DRAW
                );

                glBindVertexArray(trailVertexArrayObjects[index]);
                glDrawArrays(
                    GL_LINE_STRIP,
                    0,
                    static_cast<GLsizei>(trailVertices.size())
                );
            }
            glBindVertexArray(0);

            //Draw the same sphere mesh once for every simulated electron.
            for (const Particle& particle : electrons) {
                glm::mat4 electronModel(1.0f);

                //Move this sphere to the particle's current simulation position.
                electronModel = glm::translate(
                    electronModel,
                    particle.position
                );

                shader.setMat4("model", electronModel);
                shader.setVec3("particleColor", particle.color);
                shader.setFloat("particleColorWeight", 0.75f);
                shader.setFloat("brightness", 1.20f);
                electron.draw();
            }

            //Create a readable legend of the values currently driving the scene.
            ImGui::SetNextWindowPos(
                ImVec2(16.0f, 16.0f),
                ImGuiCond_Once
            );
            ImGui::SetNextWindowSize(
                ImVec2(310.0f, 0.0f),
                ImGuiCond_Once
            );

            ImGui::Begin(
                "Simulation Control",
                nullptr,
                ImGuiWindowFlags_NoCollapse
            );
            ImGui::Text("Electromagnetic Field");
            ImGui::Separator();

            ImGui::TextColored(
                ImVec4(1.0f, 0.66f, 0.18f, 1.0f),
                "Electric field (E)"
            );
            ImGui::Text(
                "(%.2f, %.2f, %.2f)",
                field.electric.x,
                field.electric.y,
                field.electric.z
            );

            ImGui::TextColored(
                ImVec4(0.28f, 0.90f, 1.0f, 1.0f),
                "Magnetic field (B)"
            );
            ImGui::Text(
                "(%.2f, %.2f, %.2f)",
                field.magnetic.x,
                field.magnetic.y,
                field.magnetic.z
            );

            ImGui::Spacing();
            ImGui::Text("Electron data");
            ImGui::Separator();
            ImGui::Text("Particles: %d", static_cast<int>(electrons.size()));
            ImGui::Text("Charge: %.1f", electrons.front().charge);
            ImGui::Text("Mass: %.1f", electrons.front().mass);
            ImGui::Text(
                "Speed: %.3f",
                glm::length(electrons.front().velocity)
            );
            ImGui::Text("Time step: %.4f s", physicsTimeStep);
            ImGui::Text("Trail points: %d", static_cast<int>(
                electrons.front().trail.size()
            ));

            ImGui::Spacing();
            ImGui::Text("Scene key");
            ImGui::Separator();
            ImGui::TextColored(
                ImVec4(0.28f, 0.90f, 1.0f, 1.0f),
                "Cyan lines: magnetic field"
            );
            ImGui::Text("Coloured spheres: electrons");
            ImGui::Text("Coloured paths: trajectories");

            ImGui::Spacing();
            ImGui::TextDisabled("Drag left mouse: orbit camera");
            ImGui::TextDisabled("Scroll: zoom   Esc: close");
            ImGui::End();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(
                ImGui::GetDrawData()
            );

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        glDeleteBuffers(1, &gridVertexBufferObject);
        glDeleteVertexArrays(1, &gridVertexArrayObject);
        glDeleteBuffers(1, &fieldVertexBufferObject);
        glDeleteVertexArrays(1, &fieldVertexArrayObject);
        glDeleteBuffers(
            static_cast<GLsizei>(trailVertexBufferObjects.size()),
            trailVertexBufferObjects.data()
        );
        glDeleteVertexArrays(
            static_cast<GLsizei>(trailVertexArrayObjects.size()),
            trailVertexArrayObjects.data()
        );
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        exitCode = 1;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();


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
