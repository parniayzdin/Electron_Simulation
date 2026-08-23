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
#include "AtomOverview.hpp"
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
constexpr float ATOM_OVERVIEW_DISTANCE = 10.0f;


//Purpose:
// Check keyboard
// Clear old picture
// Draw new picture
// Show it on screen
// Repeat
//Stores one corner of the flat screen-sized drawing surface.
struct ScreenVertex {
    float position[2];
    float textureCoordinate[2];
};

//Makes the hidden scene texture match the current window size.
void resizeSceneTexture(
    GLuint sceneColorTexture,
    GLuint depthStencilRenderbuffer,
    int width,
    int height
) {
    glBindTexture(GL_TEXTURE_2D, sceneColorTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        width,
        height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glBindRenderbuffer(
        GL_RENDERBUFFER,
        depthStencilRenderbuffer
    );
    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH24_STENCIL8,
        width,
        height
    );
}

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

//Creates one long line showing the tilted magnetic-field direction.
std::vector<Vertex> createFieldVertices(
    const ElectromagneticField& field,
    float
) {
    std::vector<Vertex> vertices;

    const glm::vec3 magneticUnitDirection =
        glm::normalize(field.magnetic);

    const glm::vec3 magneticColor(0.18f, 0.48f, 1.0f);

    //The previous grid of short field lines is kept here for reference.
    /*
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
    */

    //This single line is the axis that the electron should coil around.
    addLine(
        vertices,
        magneticUnitDirection * -5.0f,
        magneticUnitDirection * 5.0f,
        magneticColor
    );

    return vertices;
}

//Builds a helical starting state for any nonzero magnetic-field direction.
Particle createHelicalElectron(const ElectromagneticField& field)
{
    Particle electron;

    //These describe the demonstration, not a hard-coded path.
    constexpr float HELIX_RADIUS = 0.65f;
    constexpr float FORWARD_SPEED = 0.18f;
    constexpr float MINIMUM_FIELD_STRENGTH = 0.0001f;

    const float magneticStrength = glm::length(field.magnetic);
    if (magneticStrength < MINIMUM_FIELD_STRENGTH) {
        //A helix needs a magnetic field, so this safe fallback does not move.
        return electron;
    }

    const glm::vec3 magneticUnitDirection =
        field.magnetic / magneticStrength;

    //Choose a helper that is not parallel to B, even when B is nearly vertical.
    glm::vec3 helper(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(magneticUnitDirection, helper)) > 0.90f) {
        helper = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    //This offset points from the magnetic-field axis to the electron.
    const glm::vec3 radialOffset = glm::normalize(
        glm::cross(magneticUnitDirection, helper)
    ) * HELIX_RADIUS;

    electron.position = radialOffset;

    //This velocity makes magnetic force point back toward the field axis.
    const glm::vec3 circularVelocity =
        -(electron.charge / electron.mass) *
        glm::cross(field.magnetic, radialOffset);

    //This extra velocity carries the electron along the tilted field line.
    const glm::vec3 forwardVelocity =
        magneticUnitDirection * FORWARD_SPEED;

    electron.velocity = circularVelocity + forwardVelocity;

    return electron;
}

//Keeps a short history so each electron leaves a visible path behind it.
void recordTrailPoint(Particle& particle)
{
    constexpr std::size_t MAX_TRAIL_POINTS = 360;
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
std::vector<Vertex> createTrailVertices(
    const Particle& particle,
    float currentTime
)
{
    std::vector<Vertex> vertices;
    vertices.reserve(particle.trail.size());

    const float pulsePosition = std::fmod(currentTime * 0.65f, 1.0f);

    for (std::size_t index = 0;
         index < particle.trail.size();
         ++index) {
        const float progress = particle.trail.size() > 1 ?
            static_cast<float>(index) /
            static_cast<float>(particle.trail.size() - 1) :
            0.0f;
        const float distanceFromPulse = std::abs(
            progress - pulsePosition
        );
        const float pulseStrength = std::exp(
            -100.0f * distanceFromPulse * distanceFromPulse
        );

        //Most of the path is dim cyan; one moving section becomes bright.
        const glm::vec3 dimColor = particle.color * 0.28f;
        const glm::vec3 lightColor(0.92f, 1.0f, 1.0f);
        const glm::vec3 color = dimColor +
            (lightColor - dimColor) * pulseStrength;

        const glm::vec3& position = particle.trail[index];
        vertices.push_back({
            {position.x, position.y, position.z},
            {color.r, color.g, color.b}
        });
    }

    return vertices;
}

//Stores the camera state that changes when the user moves the mouse.
struct OrbitCamera {
    float yawDegrees = 32.0f;
    float pitchDegrees = 20.0f;
    float distance = 12.0f;

    bool isDragging = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

    glm::mat4 viewMatrix(const glm::vec3& target) const
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
        Shader lensShader(
            "shaders/screen.vert",
            "shaders/screen.frag"
        );

        // The temporary triangle is now a small, round electron.
        Sphere electron(0.18f, 32, 20);

        //All electrons see the same uniform electric and magnetic fields.
        const ElectromagneticField field;

        //This helper calculates the starting position and velocity from B.
        std::vector<Particle> electrons = {
            createHelicalElectron(field)
        };

        const std::vector<Vertex> gridVertices =
            createGridVertices();
        const AtomOverview atomOverview;
        const std::vector<Vertex>& orbitalVertices =
            atomOverview.orbitalVertices();
        const std::vector<Vertex>& clippingPlaneVertices =
            atomOverview.clippingPlaneVertices();

        const std::vector<Vertex> fieldVertices =
            createFieldVertices(field, 0.0f);

        GLuint gridVertexArrayObject = 0;
        GLuint gridVertexBufferObject = 0;
        GLuint hydrogenCloudVertexArrayObject = 0;
        GLuint hydrogenCloudVertexBufferObject = 0;
        GLuint clippingPlaneVertexArrayObject = 0;
        GLuint clippingPlaneVertexBufferObject = 0;
        GLuint fieldVertexArrayObject = 0;
        GLuint fieldVertexBufferObject = 0;
        GLuint screenVertexArrayObject = 0;
        GLuint screenVertexBufferObject = 0;
        GLuint sceneFramebuffer = 0;
        GLuint sceneColorTexture = 0;
        GLuint sceneDepthStencilRenderbuffer = 0;
        int sceneTextureWidth = 0;
        int sceneTextureHeight = 0;
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

        //The orbital point cloud uses the same position-and-colour layout.
        glGenVertexArrays(1, &hydrogenCloudVertexArrayObject);
        glGenBuffers(1, &hydrogenCloudVertexBufferObject);
        glBindVertexArray(hydrogenCloudVertexArrayObject);
        glBindBuffer(GL_ARRAY_BUFFER, hydrogenCloudVertexBufferObject);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                orbitalVertices.size() * sizeof(Vertex)
            ),
            orbitalVertices.data(),
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

        //The three white guide planes also use the same Vertex layout.
        glGenVertexArrays(1, &clippingPlaneVertexArrayObject);
        glGenBuffers(1, &clippingPlaneVertexBufferObject);
        glBindVertexArray(clippingPlaneVertexArrayObject);
        glBindBuffer(GL_ARRAY_BUFFER, clippingPlaneVertexBufferObject);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                clippingPlaneVertices.size() * sizeof(Vertex)
            ),
            clippingPlaneVertices.data(),
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

        //The field line uses the same Vertex structure as the grid.
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

        //This flat rectangle displays the completed 3D scene after lensing it.
        const ScreenVertex screenVertices[] = {
            {{-1.0f, -1.0f}, {0.0f, 0.0f}},
            {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
            {{ 1.0f,  1.0f}, {1.0f, 1.0f}},
            {{-1.0f, -1.0f}, {0.0f, 0.0f}},
            {{ 1.0f,  1.0f}, {1.0f, 1.0f}},
            {{-1.0f,  1.0f}, {0.0f, 1.0f}}
        };

        glGenVertexArrays(1, &screenVertexArrayObject);
        glGenBuffers(1, &screenVertexBufferObject);
        glBindVertexArray(screenVertexArrayObject);
        glBindBuffer(GL_ARRAY_BUFFER, screenVertexBufferObject);
        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(screenVertices),
            screenVertices,
            GL_STATIC_DRAW
        );
        glVertexAttribPointer(
            0,
            2,
            GL_FLOAT,
            GL_FALSE,
            static_cast<GLsizei>(sizeof(ScreenVertex)),
            reinterpret_cast<void*>(
                offsetof(ScreenVertex, position)
            )
        );
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            1,
            2,
            GL_FLOAT,
            GL_FALSE,
            static_cast<GLsizei>(sizeof(ScreenVertex)),
            reinterpret_cast<void*>(
                offsetof(ScreenVertex, textureCoordinate)
            )
        );
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        //Render the 3D scene to a hidden texture before showing it on screen.
        glGenFramebuffers(1, &sceneFramebuffer);
        glGenTextures(1, &sceneColorTexture);
        glGenRenderbuffers(1, &sceneDepthStencilRenderbuffer);

        glBindTexture(GL_TEXTURE_2D, sceneColorTexture);
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR
        );
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR
        );
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE
        );
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE
        );
        resizeSceneTexture(
            sceneColorTexture,
            sceneDepthStencilRenderbuffer,
            WINDOW_WIDTH,
            WINDOW_HEIGHT
        );
        sceneTextureWidth = WINDOW_WIDTH;
        sceneTextureHeight = WINDOW_HEIGHT;

        glBindFramebuffer(GL_FRAMEBUFFER, sceneFramebuffer);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            sceneColorTexture,
            0
        );
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER,
            sceneDepthStencilRenderbuffer
        );
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error("Could not create scene framebuffer.");
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

            //Scrolling far away shows the atom; scrolling close shows the field.
            const bool isAtomOverview = camera.distance >=
                ATOM_OVERVIEW_DISTANCE;

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

            //Pause the close-up physics while the user is viewing the atom.
            if (!isAtomOverview) {
                for (Particle& particle : electrons) {
                    advanceParticleWithBoris(
                        particle,
                        field,
                        physicsTimeStep
                    );
                    recordTrailPoint(particle);
                }
            }

            int framebufferWidth = 0;
            int framebufferHeight = 0;
            glfwGetFramebufferSize(
                window,
                &framebufferWidth,
                &framebufferHeight
            );

            //Keep the hidden scene texture sharp when the window is resized.
            if (framebufferWidth != sceneTextureWidth ||
                framebufferHeight != sceneTextureHeight) {
                resizeSceneTexture(
                    sceneColorTexture,
                    sceneDepthStencilRenderbuffer,
                    framebufferWidth,
                    framebufferHeight
                );
                sceneTextureWidth = framebufferWidth;
                sceneTextureHeight = framebufferHeight;
            }

            //First draw the regular 3D world into a hidden texture.
            glBindFramebuffer(GL_FRAMEBUFFER, sceneFramebuffer);
            glViewport(0, 0, framebufferWidth, framebufferHeight);
            glClear(
                GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT
            );

            shader.use();

            //The atom view looks at the nucleus; field view follows the helix axis.
            const glm::vec3 magneticUnitDirection =
                glm::normalize(field.magnetic);
            const glm::vec3 fieldAxisCenter = magneticUnitDirection *
                glm::dot(
                    electrons.front().position,
                    magneticUnitDirection
                );
            const glm::vec3 viewTarget = isAtomOverview ?
                glm::vec3(0.0f) : fieldAxisCenter;
            const glm::mat4 view = camera.viewMatrix(viewTarget);
            shader.setMat4("view", view);

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

            if (isAtomOverview) {
                //Draw a fixed higher-orbital probability-density overview.
                const glm::mat4 atomModel(1.0f);
                shader.setMat4("model", atomModel);
                shader.setFloat("particleColorWeight", 0.0f);

                shader.setFloat("brightness", 1.0f);
                glPointSize(2.2f);
                glBindVertexArray(hydrogenCloudVertexArrayObject);
                glDrawArrays(
                    GL_POINTS,
                    0,
                    static_cast<GLsizei>(orbitalVertices.size())
                );
                glBindVertexArray(0);
                glPointSize(1.0f);

                //Draw guide planes last so their outlines stay visible.
                glDisable(GL_DEPTH_TEST);
                shader.setFloat("brightness", 1.0f);
                glBindVertexArray(clippingPlaneVertexArrayObject);
                glDrawArrays(
                    GL_LINES,
                    0,
                    static_cast<GLsizei>(clippingPlaneVertices.size())
                );
                glBindVertexArray(0);
                glEnable(GL_DEPTH_TEST);
            }
            else {
                //Move the field grid with the helix axis as the camera follows it.
                const glm::mat4 fieldModel = glm::translate(
                    glm::mat4(1.0f),
                    fieldAxisCenter
                );
                shader.setMat4("model", fieldModel);
                shader.setFloat("particleColorWeight", 0.0f);
                shader.setFloat("brightness", 1.0f);

                glBindVertexArray(gridVertexArrayObject);
                glDrawArrays(
                    GL_LINES,
                    0,
                    static_cast<GLsizei>(gridVertices.size())
                );
                glBindVertexArray(0);

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

                shader.setMat4("model", glm::mat4(1.0f));
                shader.setFloat("brightness", 1.35f);
                for (std::size_t index = 0;
                     index < electrons.size();
                     ++index) {
                    const std::vector<Vertex> trailVertices =
                        createTrailVertices(
                            electrons[index],
                            currentTime
                        );

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

                for (const Particle& particle : electrons) {
                    glm::mat4 electronModel(1.0f);
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
            }

            //Choose a short part of the trail that is safely behind the sphere.
            glm::vec2 trailStart(-2.0f, -2.0f);
            glm::vec2 trailEnd(-2.0f, -2.0f);
            float distortionStrength = 0.0f;
            constexpr std::size_t POINTS_BEHIND_ELECTRON = 24;
            constexpr std::size_t DISTORTION_TRAIL_LENGTH = 28;

            if (!isAtomOverview &&
                electrons.front().trail.size() >
                POINTS_BEHIND_ELECTRON + 1) {
                const std::size_t trailEndIndex =
                    electrons.front().trail.size() - 1 -
                    POINTS_BEHIND_ELECTRON;
                const std::size_t trailStartIndex = trailEndIndex >
                    DISTORTION_TRAIL_LENGTH ?
                    trailEndIndex - DISTORTION_TRAIL_LENGTH : 0;

                const glm::vec4 trailStartClip = projection * view *
                    glm::vec4(
                        electrons.front().trail[trailStartIndex],
                        1.0f
                    );
                const glm::vec4 trailEndClip = projection * view *
                    glm::vec4(
                        electrons.front().trail[trailEndIndex],
                        1.0f
                    );

                if (trailStartClip.w > 0.0f &&
                    trailEndClip.w > 0.0f) {
                    const glm::vec3 startNormalized =
                        glm::vec3(trailStartClip) / trailStartClip.w;
                    const glm::vec3 endNormalized =
                        glm::vec3(trailEndClip) / trailEndClip.w;
                    trailStart = glm::vec2(
                        startNormalized.x * 0.5f + 0.5f,
                        startNormalized.y * 0.5f + 0.5f
                    );
                    trailEnd = glm::vec2(
                        endNormalized.x * 0.5f + 0.5f,
                        endNormalized.y * 0.5f + 0.5f
                    );
                    distortionStrength = 1.0f;
                }
            }

            //Show the hidden scene with a thin distortion along the trail.
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, framebufferWidth, framebufferHeight);
            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            lensShader.use();
            lensShader.setInt("sceneTexture", 0);
            lensShader.setVec2("trailStart", trailStart);
            lensShader.setVec2("trailEnd", trailEnd);
            lensShader.setFloat("animationTime", currentTime);
            lensShader.setFloat(
                "distortionStrength",
                distortionStrength
            );
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sceneColorTexture);
            glBindVertexArray(screenVertexArrayObject);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
            glEnable(GL_DEPTH_TEST);

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

            if (isAtomOverview) {
                ImGui::Text("Hydrogen Orbital Probability View");
                ImGui::Separator();
                ImGui::Text("Fixed higher-orbital visual");
                ImGui::Text("Warm points: probability density");
                ImGui::Text("White outlines: clipping planes");
                ImGui::Spacing();
                ImGui::TextDisabled(
                    "Scroll up to enter the field simulation"
                );
                if (ImGui::Button("Zoom into electron field")) {
                    camera.distance = ATOM_OVERVIEW_DISTANCE - 0.5f;
                }
            }
            else {
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
                ImGui::TextDisabled(
                    "Scroll down to return to the atom overview"
                );
                if (ImGui::Button("Zoom out to atom")) {
                    camera.distance = ATOM_OVERVIEW_DISTANCE + 0.5f;
                }
            }

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
        glDeleteBuffers(1, &hydrogenCloudVertexBufferObject);
        glDeleteVertexArrays(1, &hydrogenCloudVertexArrayObject);
        glDeleteBuffers(1, &clippingPlaneVertexBufferObject);
        glDeleteVertexArrays(1, &clippingPlaneVertexArrayObject);
        glDeleteBuffers(1, &fieldVertexBufferObject);
        glDeleteVertexArrays(1, &fieldVertexArrayObject);
        glDeleteBuffers(1, &screenVertexBufferObject);
        glDeleteVertexArrays(1, &screenVertexArrayObject);
        glDeleteRenderbuffers(1, &sceneDepthStencilRenderbuffer);
        glDeleteTextures(1, &sceneColorTexture);
        glDeleteFramebuffers(1, &sceneFramebuffer);
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
