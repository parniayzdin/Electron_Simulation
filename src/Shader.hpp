#pragma once

#include <GL/glew.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

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

    //Sends one RGB colour to a shader uniform.
    void setVec3(
        const std::string& name,
        const glm::vec3& value
    ) const;

    //Sends one decimal value to a shader uniform.
    void setFloat(
        const std::string& name,
        float value
    ) const;

private:
    GLuint programId = 0;
};
