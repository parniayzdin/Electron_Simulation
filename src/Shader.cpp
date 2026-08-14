#include "Shader.hpp"

#include <fstream>
#include <glm/mat4x4.hpp>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
//How things should be draw on the screen, the order of the vertices, and how to draw them.
std::string readFile(const std::string& path)
{
    std::ifstream file(path);

    if (!file.is_open()) {
        throw std::runtime_error(
            "Could not open shader file: " + path
        );
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

//Compiles either a vertex shader or a fragment shader.
GLuint compileShader(
    GLenum shaderType,
    const std::string& source
)
{
    const GLuint shaderId = glCreateShader(shaderType);

    const char* sourcePointer = source.c_str();

    glShaderSource(
        shaderId,
        1,
        &sourcePointer,
        nullptr
    );

    glCompileShader(shaderId);

    GLint compilationSucceeded = GL_FALSE;

    glGetShaderiv(
        shaderId,
        GL_COMPILE_STATUS,
        &compilationSucceeded
    );

    if (compilationSucceeded == GL_FALSE) {
        GLint messageLength = 0;

        glGetShaderiv(
            shaderId,
            GL_INFO_LOG_LENGTH,
            &messageLength
        );

        std::vector<char> message(messageLength);

        glGetShaderInfoLog(
            shaderId,
            messageLength,
            nullptr,
            message.data()
        );

        glDeleteShader(shaderId);

        throw std::runtime_error(
            "Shader compilation failed:\n" +
            std::string(message.data())
        );
    }

    return shaderId;
}

} 

Shader::Shader(
    const std::string& vertexPath,
    const std::string& fragmentPath
)
{
    // Read both shader files.
    const std::string vertexCode = readFile(vertexPath);
    const std::string fragmentCode = readFile(fragmentPath);

    const GLuint vertexShader = compileShader(
        GL_VERTEX_SHADER,
        vertexCode
    );

    const GLuint fragmentShader = compileShader(
        GL_FRAGMENT_SHADER,
        fragmentCode
    );

    programId = glCreateProgram();

    glAttachShader(programId, vertexShader);
    glAttachShader(programId, fragmentShader);
    glLinkProgram(programId);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linkingSucceeded = GL_FALSE;

    glGetProgramiv(
        programId,
        GL_LINK_STATUS,
        &linkingSucceeded
    );

    if (linkingSucceeded == GL_FALSE) {
        GLint messageLength = 0;

        glGetProgramiv(
            programId,
            GL_INFO_LOG_LENGTH,
            &messageLength
        );

        std::vector<char> message(messageLength);

        glGetProgramInfoLog(
            programId,
            messageLength,
            nullptr,
            message.data()
        );

        glDeleteProgram(programId);
        programId = 0;

        throw std::runtime_error(
            "Shader linking failed:\n" +
            std::string(message.data())
        );
    }
}

Shader::~Shader()
{
    if (programId != 0) {
        glDeleteProgram(programId);
    }
}

void Shader::use() const
{
    glUseProgram(programId);
}

//Sends a 4x4 matrix to the shader program
void setMax4(
    const std::string& name,
    const glm::mat4& matrix
)const;


// Rotate the object slowly around the Y axis.
glm::mat4 model(1.0f);

model = glm::rotate(
    model,
    static_cast<float>(glfwGetTime()),
    glm::vec3(0.0f, 1.0f, 0.0f)
);

// Place the camera three units away from the triangle.
glm::mat4 view = glm::lookAt(
    glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f)
);

//Create perspective so distant objects appear smaller.
glm::mat4 projection = glm::perspective(
    glm::radians(45.0f),
    static_cast<float>(WINDOW_WIDTH) /
        static_cast<float>(WINDOW_HEIGHT),
    0.1f,
    100.0f
);

shader.setMat4("model", model);
shader.setMat4("view", view);
shader.setMat4("projection", projection);
