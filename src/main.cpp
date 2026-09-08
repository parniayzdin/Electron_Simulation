#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include "AtomOverview.hpp"
#include "OrbitCamera.hpp"
#include "Shader.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct OrbitalChoice { const char* name; int n, l, m; };
constexpr std::array<OrbitalChoice, 6> orbitals {{
    {"1s / ground state", 1, 0, 0}, {"2s / radial node", 2, 0, 0},
    {"2p / two lobes", 2, 1, 0}, {"3d / four lobes", 3, 2, 2},
    {"3d / axial", 3, 2, 0}, {"4f / original cloud", 4, 3, 2}
}};

// Own GPU geometry so cleanup also runs when shader loading or rendering fails.
class PointMesh {
public:
    PointMesh() {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void*>(offsetof(Vertex, position)));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void*>(offsetof(Vertex, color)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }
    ~PointMesh() { glDeleteBuffers(1, &vbo); glDeleteVertexArrays(1, &vao); }
    PointMesh(const PointMesh&) = delete;
    PointMesh& operator=(const PointMesh&) = delete;
    void upload(const std::vector<Vertex>& points) {
        count = static_cast<GLsizei>(points.size());
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Vertex),
                     points.data(), GL_STATIC_DRAW);
    }
    void draw(GLenum primitive = GL_POINTS) const {
        glBindVertexArray(vao);
        glDrawArrays(primitive, 0, count);
        glBindVertexArray(0);
    }
private:
    GLuint vao = 0, vbo = 0;
    GLsizei count = 0;
};

bool panelCapturesMouse() {
    return ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse;
}

void mouseButton(GLFWwindow* window, int button, int action, int) {
    auto& camera = *static_cast<OrbitCamera*>(glfwGetWindowUserPointer(window));
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    // Always process release, even if a drag ends over the panel.
    if (action == GLFW_RELEASE) camera.isDragging = false;
    else if (action == GLFW_PRESS && !panelCapturesMouse()) {
        camera.isDragging = true;
        glfwGetCursorPos(window, &camera.lastMouseX, &camera.lastMouseY);
    }
}

void cursorPosition(GLFWwindow* window, double x, double y) {
    auto& camera = *static_cast<OrbitCamera*>(glfwGetWindowUserPointer(window));
    const float dx = static_cast<float>(x - camera.lastMouseX);
    const float dy = static_cast<float>(camera.lastMouseY - y);
    camera.lastMouseX = x;
    camera.lastMouseY = y;
    if (!camera.isDragging || panelCapturesMouse()) return;
    camera.yawDegrees = std::remainder(camera.yawDegrees + dx * 0.25f, 360.0f);
    camera.pitchDegrees = std::clamp(camera.pitchDegrees + dy * 0.25f, -89.0f, 89.0f);
}

void scroll(GLFWwindow* window, double, double amount) {
    if (!panelCapturesMouse())
        static_cast<OrbitCamera*>(glfwGetWindowUserPointer(window))->zoom(amount);
}

void focusChanged(GLFWwindow* window, int focused) {
    if (!focused)
        static_cast<OrbitCamera*>(glfwGetWindowUserPointer(window))->isDragging = false;
}

std::filesystem::path shaderDirectory(const char* argv0) {
    // Launching from a file manager or another working directory also works.
    const auto besideExecutable = std::filesystem::absolute(argv0).parent_path() / "shaders";
    if (std::filesystem::exists(besideExecutable / "atom.vert")) return besideExecutable;
    if (std::filesystem::exists("shaders/atom.vert")) return "shaders";
#ifdef ELECTRON_SHADER_DIR
    if (std::filesystem::exists(std::filesystem::path(ELECTRON_SHADER_DIR) / "atom.vert"))
        return ELECTRON_SHADER_DIR;
#endif
    throw std::runtime_error("Cannot find shaders beside the executable.");
}

void captureFrame(const std::filesystem::path& path, int width, int height) {
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("Cannot write screenshot: " + path.string());
    output << "P6\n" << width << ' ' << height << "\n255\n";
    for (int y = height - 1; y >= 0; --y)
        output.write(reinterpret_cast<const char*>(pixels.data() +
            static_cast<std::size_t>(y) * width * 3), width * 3);
    if (!output) throw std::runtime_error("Screenshot write failed.");
}

int run(GLFWwindow* window, OrbitCamera& camera, const char* argv0,
        const std::filesystem::path& smokeDirectory) {
    const auto shaderPath = shaderDirectory(argv0);
    Shader points((shaderPath / "atom.vert").string(), (shaderPath / "atom.frag").string());
    Shader lines((shaderPath / "basic.vert").string(), (shaderPath / "basic.frag").string());
    PointMesh cloud, nucleus, guides;
    int selectedOrbital = 5;
    float fitDistance = 12.0f;
    auto loadOrbital = [&] {
        const auto& state = orbitals[selectedOrbital];
        AtomOverview atom(state.n, state.l, state.m);
        cloud.upload(atom.orbitalVertices());
        nucleus.upload(atom.nucleusVertices());
        guides.upload(atom.clippingPlaneVertices());
        fitDistance = atom.overviewDistance();
    };
    loadOrbital();

    bool showNucleus = true, showGuides = false, cutaway = false, autoRotate = false;
    float cloudOpacity = 0.38f, dotSize = 1.0f, cutPosition = 0.0f;
    float pointSizeRange[2] = {1.0f, 1.0f};
    glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, pointSizeRange);
    const float maxPointSize = std::min(7.0f, pointSizeRange[1]);
    double previousTime = glfwGetTime();
    int smokeFrame = 0;
    if (!smokeDirectory.empty()) std::filesystem::create_directories(smokeDirectory);
    const std::array<float, 6> smokeDistances {{12.0f, 9.8f, 3.0f, 0.38f, 0.28f, 1.8f}};
    const std::array<const char*, 6> smokeNames {{
        "01-overview.ppm", "02-former-switch.ppm", "03-interior.ppm",
        "04-proton.ppm", "05-minimum-zoom.ppm", "06-ground-state.ppm"
    }};

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const double now = glfwGetTime();
        const float dt = static_cast<float>(std::clamp(now - previousTime, 0.0, 0.1));
        previousTime = now;
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        // Do not allocate zero-size buffers or divide by zero while minimized.
        if (width <= 0 || height <= 0) {
            camera.isDragging = false;
            glfwWaitEventsTimeout(0.05);
            continue;
        }

        if (!smokeDirectory.empty()) {
            camera.distance = camera.requestedDistance = smokeDistances[smokeFrame];
            cutaway = smokeFrame == 2;
            if (smokeFrame == 5) { selectedOrbital = 0; loadOrbital(); }
        }
        camera.update(dt);
        if (autoRotate && !camera.isDragging)
            camera.yawDegrees = std::remainder(camera.yawDegrees + 9.0f * dt, 360.0f);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        auto& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) {
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) glfwSetWindowShouldClose(window, GLFW_TRUE);
            if (ImGui::IsKeyPressed(ImGuiKey_H)) camera.setDistance(fitDistance);
            if (ImGui::IsKeyPressed(ImGuiKey_N)) camera.setDistance(0.38f);
            if (ImGui::IsKeyPressed(ImGuiKey_Space)) autoRotate = !autoRotate;
        }

        const float panelWidth = std::min(292.0f, io.DisplaySize.x * 0.42f);
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(panelWidth, io.DisplaySize.y));
        ImGui::Begin("Hydrogen atom", nullptr,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextColored(ImVec4(1, 0.66f, 0.3f, 1), "INSIDE HYDROGEN");
        ImGui::TextWrapped("One proton. One electron. A cloud of possibilities.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Orbital");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##orbital", orbitals[selectedOrbital].name)) {
            for (int i = 0; i < static_cast<int>(orbitals.size()); ++i) {
                if (ImGui::Selectable(orbitals[i].name, i == selectedOrbital)) {
                    selectedOrbital = i;
                    loadOrbital();
                    camera.setDistance(fitDistance);
                    cutPosition = 0.0f;
                }
            }
            ImGui::EndCombo();
        }
        const auto& state = orbitals[selectedOrbital];
        ImGui::Text("n = %d   l = %d   |m| = %d", state.n, state.l, state.m);
        if (state.m > 0) ImGui::TextDisabled("Real cosine orbital combination");
        ImGui::Spacing();
        ImGui::Text("Explore");
        if (ImGui::Button("Whole atom", ImVec2(-1, 0))) camera.setDistance(fitDistance);
        if (ImGui::Button("Inside cloud", ImVec2(-1, 0)))
            camera.setDistance(std::max(0.6f, fitDistance * 0.28f));
        if (ImGui::Button("Nucleus close-up", ImVec2(-1, 0))) {
            showNucleus = true;
            camera.setDistance(0.38f);
        }
        float zoom = fitDistance / camera.requestedDistance;
        ImGui::SetNextItemWidth(-1);
        if (ImGui::SliderFloat("##zoom", &zoom, fitDistance / OrbitCamera::maximumDistance,
            fitDistance / OrbitCamera::minimumDistance, "Zoom %.1fx", ImGuiSliderFlags_Logarithmic))
            camera.setDistance(fitDistance / zoom);
        ImGui::TextDisabled("Distance: %.2f a0", camera.distance / AtomOverview::worldScale);
        ImGui::Spacing();
        ImGui::Checkbox("Show proton", &showNucleus);
        ImGui::Checkbox("Cut away front / +X half", &cutaway);
        if (cutaway) {
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("##slice", &cutPosition, -fitDistance * 0.5f,
                fitDistance * 0.5f, "Slice %.2f");
        }
        ImGui::Checkbox("Reference planes", &showGuides);
        ImGui::Checkbox("Slow orbit", &autoRotate);
        ImGui::Text("Dot appearance");
        ImGui::SetNextItemWidth(-1);
        ImGui::SliderFloat("##density", &cloudOpacity, 0.03f, 0.85f, "Brightness %.2f");
        ImGui::SetNextItemWidth(-1);
        ImGui::SliderFloat("##dots", &dotSize, 0.5f, 2.0f, "Dot size %.1f");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1, 0.48f, 0.17f, 1), "Orange center: one proton");
        ImGui::TextWrapped("The proton marker is enlarged. Its dots are a visual texture.");
        ImGui::Spacing();
        ImGui::TextWrapped("Cloud dots sample the probability of finding one electron. They are not separate electrons or a path.");
        if (state.l > 0)
            ImGui::TextWrapped("This orbital has very low probability near the nucleus. Empty regions are expected.");
        ImGui::Spacing();
        ImGui::TextDisabled("Drag: orbit  /  Scroll: zoom");
        ImGui::TextDisabled("H: whole atom  /  N: nucleus");
        ImGui::TextDisabled("Space: slow orbit  /  Esc: exit");
        ImGui::End();

        glViewport(0, 0, width, height);
        glClearColor(0.012f, 0.017f, 0.035f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const int inset = std::clamp(static_cast<int>(
            panelWidth * width / std::max(io.DisplaySize.x, 1.0f)), 0, width - 1);
        const int sceneWidth = width - inset;
        glViewport(inset, 0, sceneWidth, height);
        const auto view = camera.viewMatrix(glm::vec3(0));
        const auto projection = glm::perspective(glm::radians(45.0f),
            static_cast<float>(sceneWidth) / height, 0.002f, 100.0f);

        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        // The cloud integrates probability along the view; no order-dependent occlusion.
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        points.use();
        points.setMat4("view", view);
        points.setMat4("projection", projection);
        points.setFloat("viewportHeight", static_cast<float>(height));
        points.setFloat("maxPointSize", maxPointSize);
        points.setFloat("dotRadius", 0.014f * dotSize);
        points.setFloat("opacity", cloudOpacity);
        points.setInt("cutaway", cutaway ? 1 : 0);
        points.setFloat("cutPosition", cutPosition);
        cloud.draw();
        if (showNucleus) {
            points.setInt("cutaway", 0);
            points.setFloat("dotRadius", 0.0015f * dotSize);
            points.setFloat("opacity", 0.64f);
            nucleus.draw();
        }
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        if (showGuides) {
            lines.use();
            lines.setMat4("model", glm::scale(glm::mat4(1), glm::vec3(fitDistance / 12.0f)));
            lines.setMat4("view", view);
            lines.setMat4("projection", projection);
            lines.setFloat("particleColorWeight", 0);
            lines.setFloat("brightness", 0.32f * std::min(1.0f, camera.distance / fitDistance));
            guides.draw(GL_LINES);
        }
        glDisable(GL_PROGRAM_POINT_SIZE);

        const char* viewLabel = camera.distance < 0.8f ? "NUCLEUS / ENLARGED PROTON" :
            (camera.distance < fitDistance * 0.6f ? "INSIDE THE PROBABILITY CLOUD" :
             "HYDROGEN / ORBITAL OVERVIEW");
        ImGui::GetForegroundDrawList()->AddText(
            ImVec2(panelWidth + 20, 20), IM_COL32(210, 215, 232, 255), viewLabel);
        ImGui::Render();
        glViewport(0, 0, width, height);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (!smokeDirectory.empty()) {
            const GLenum error = glGetError();
            if (error != GL_NO_ERROR)
                throw std::runtime_error("OpenGL smoke test error: " + std::to_string(error));
            captureFrame(smokeDirectory / smokeNames[smokeFrame], width, height);
            ++smokeFrame;
            if (smokeFrame == static_cast<int>(smokeDistances.size()))
                glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        glfwSwapBuffers(window);
    }
    return 0;
}
} // namespace

int main(int argc, char** argv) {
    std::filesystem::path smokeDirectory;
    if (argc == 3 && std::string(argv[1]) == "--smoke-test") smokeDirectory = argv[2];
    else if (argc > 1) {
        std::cout << "Usage: Electron_Simulation [--smoke-test output-directory]\n";
        return std::string(argv[1]) == "--help" ? 0 : 1;
    }
    glfwSetErrorCallback([](int code, const char* message) {
        std::cerr << "GLFW " << code << ": " << message << '\n';
    });
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if (!smokeDirectory.empty()) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(1200, 800, "Hydrogen Atom Explorer", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    // Some core-profile drivers leave GL_INVALID_ENUM after GLEW initialization.
    while (glGetError() != GL_NO_ERROR) {}
    glfwSwapInterval(smokeDirectory.empty() ? 1 : 0);
    OrbitCamera camera;
    glfwSetWindowUserPointer(window, &camera);
    glfwSetMouseButtonCallback(window, mouseButton);
    glfwSetCursorPosCallback(window, cursorPosition);
    glfwSetScrollCallback(window, scroll);
    glfwSetWindowFocusCallback(window, focusChanged);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(16, 16);
    style.FramePadding = ImVec2(9, 6);
    style.ItemSpacing = ImVec2(8, 9);
    style.FrameRounding = 5;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.055f, 0.09f, 1);
    style.Colors[ImGuiCol_Button] = ImVec4(0.15f, 0.20f, 0.30f, 1);
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    int result = 0;
    try { result = run(window, camera, argv[0], smokeDirectory); }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
