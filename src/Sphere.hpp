#pragma once

#include <GL/glew.h>

// A reusable GPU-backed sphere. We will use one as the visible electron.
class Sphere {
public:
    Sphere(float radius, int sectorCount, int stackCount);
    ~Sphere();

    Sphere(const Sphere&) = delete;
    Sphere& operator=(const Sphere&) = delete;

    // Draws the triangles that make up the sphere.
    void draw() const;

private:
    GLuint vertexArrayObject = 0;
    GLuint vertexBufferObject = 0;
    GLuint indexBufferObject = 0;
    GLsizei indexCount = 0;
};
