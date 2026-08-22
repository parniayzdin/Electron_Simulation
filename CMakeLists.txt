cmake_minimum_required(VERSION 3.14)
project(Electron_Simulation VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(GLEW REQUIRED glew)
pkg_check_modules(GLM REQUIRED glm)
find_package(OpenGL REQUIRED)

include(FetchContent)

#Download Dear ImGui during CMake configuration so the control panel is reproducible.
FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.6.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLEW)

target_link_libraries(imgui PUBLIC
    ${GLFW_LIBRARIES}
    ${OPENGL_gl_LIBRARY}
)

include_directories(
    ${GLFW_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
)
link_directories(
    ${GLFW_LIBRARY_DIRS}
    ${GLEW_LIBRARY_DIRS}
    ${GLM_LIBRARY_DIRS}
)

add_executable(Electron_Simulation
    src/main.cpp
    src/Shader.cpp
    src/Sphere.cpp
    src/atom.cpp
    src/Physics.cpp
)

target_compile_definitions(Electron_Simulation PRIVATE GLFW_INCLUDE_NONE)

target_link_libraries(Electron_Simulation PRIVATE
    imgui
    ${GLFW_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${GLM_LIBRARIES}
    ${OPENGL_gl_LIBRARY}
)

add_custom_command(TARGET Electron_Simulation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Electron_Simulation>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:Electron_Simulation>/shaders
)
