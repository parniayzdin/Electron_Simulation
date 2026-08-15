#include "Sphere.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace {

constexpr float PI = 3.14159265358979323846f;

// This layout matches locations 0 and 1 in basic.vert.
struct SphereVertex {
    float position[3];
    float color[3];
};

} // namespace

Sphere::Sphere(float radius, int sectorCount, int stackCount)
{
    if (radius <= 0.0f || sectorCount < 3 || stackCount < 2) {
        throw std::invalid_argument(
            "Sphere needs a positive radius and enough segments."
        );
    }

    std::vector<SphereVertex> vertices;
    std::vector<unsigned int> indices;

    // Stacks move from the top of the sphere to its bottom.
    for (int stack = 0; stack <= stackCount; ++stack) {
        const float stackAngle =
            (PI / 2.0f) -
            (static_cast<float>(stack) * PI / stackCount);

        const float y = radius * std::sin(stackAngle);
        const float ringRadius = radius * std::cos(stackAngle);

        // Sectors travel around one horizontal ring.
        for (int sector = 0;
             sector <= sectorCount;
             ++sector) {
            const float sectorAngle =
                static_cast<float>(sector) *
                (2.0f * PI / sectorCount);

            const float x = ringRadius * std::cos(sectorAngle);
            const float z = ringRadius * std::sin(sectorAngle);

            // A cyan-to-blue gradient makes the electron easier to see.
            const float height = (y / radius + 1.0f) / 2.0f;

            vertices.push_back({
                {x, y, z},
                {0.12f + 0.18f * height,
                 0.55f + 0.35f * height,
                 1.0f}
            });
        }
    }

    // Join neighbouring rings. Every rectangular section becomes two triangles.
    for (int stack = 0; stack < stackCount; ++stack) {
        const unsigned int currentRing =
            static_cast<unsigned int>(stack * (sectorCount + 1));

        const unsigned int nextRing =
            currentRing + static_cast<unsigned int>(sectorCount + 1);

        for (int sector = 0; sector < sectorCount; ++sector) {
            const unsigned int topLeft = currentRing + sector;
            const unsigned int bottomLeft = nextRing + sector;
            const unsigned int topRight = topLeft + 1;
            const unsigned int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    indexCount = static_cast<GLsizei>(indices.size());

    // VAO remembers this sphere's vertex layout.
    glGenVertexArrays(1, &vertexArrayObject);
    glGenBuffers(1, &vertexBufferObject);
    glGenBuffers(1, &indexBufferObject);

    glBindVertexArray(vertexArrayObject);

    // VBO stores the unique vertex positions and colours on the GPU.
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            vertices.size() * sizeof(SphereVertex)
        ),
        vertices.data(),
        GL_STATIC_DRAW
    );

    // EBO says which three vertices make each triangle.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferObject);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            indices.size() * sizeof(unsigned int)
        ),
        indices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(SphereVertex)),
        reinterpret_cast<void*>(offsetof(SphereVertex, position))
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(SphereVertex)),
        reinterpret_cast<void*>(offsetof(SphereVertex, color))
    );
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

Sphere::~Sphere()
{
    glDeleteBuffers(1, &indexBufferObject);
    glDeleteBuffers(1, &vertexBufferObject);
    glDeleteVertexArrays(1, &vertexArrayObject);
}

void Sphere::draw() const
{
    glBindVertexArray(vertexArrayObject);

    glDrawElements(
        GL_TRIANGLES,
        indexCount,
        GL_UNSIGNED_INT,
        nullptr
    );

    glBindVertexArray(0);
}
