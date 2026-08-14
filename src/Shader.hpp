#pragma once
#include <GL/glew.h>
#include <string>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    void use() const;

private:
    GLuint programId = 0;
};

void Shader::setMat4(
    const std::string& name,
    const glm::mat4& matrix
) const
{
    const GLint location = glGetUniformLocation(
        programId,
        name.c_str()
    );

    glUniformMatrix4fv(
        location,
        1,
        GL_FALSE,
        glm::value_ptr(matrix)
    );
}