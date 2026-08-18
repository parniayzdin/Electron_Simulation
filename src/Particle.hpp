#pragma once //Prevents this header from being included more than once.

#include <glm/glm.hpp>

struct Particle {
    glm::vec3 position = glm::vec3(0.0f); //Current x, y, and z location.
    glm::vec3 velocity = glm::vec3(0.0f); //Movement per second.
    glm::vec3 color = glm::vec3(0.15f, 0.80f, 1.0f); //Future display colour.
    float charge = -1.0f; //Negative normalized electron charge.
    float mass = 1.0f; //Normalized mass for simple simulation units.
};

