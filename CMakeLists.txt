cmake_minimum_required(VERSION 3.10)
project(Electron_Simulation VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(GLEW REQUIRED glew)
pkg_check_modules(GLM REQUIRED glm)
find_package(OpenGL REQUIRED)

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
    src/atom.cpp
)

target_compile_definitions(Electron_Simulation PRIVATE GLFW_INCLUDE_NONE)

target_link_libraries(Electron_Simulation PRIVATE
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
