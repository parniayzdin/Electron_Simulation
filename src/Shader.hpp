#pragma once

#include <GL/glew.h>

#include <glm/mat4x4.hpp>

#include <string>

class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;

    // Sends a transformation matrix to a named shader uniform.
    void setMat4(
        const std::string& name,
        const glm::mat4& matrix
    ) const;

private:
    GLuint programId = 0;
};
