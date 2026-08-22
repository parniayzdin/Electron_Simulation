#pragma once

#include <glm/glm.hpp>

//Stores the two uniform fields used by the whole simulation scene.
struct ElectromagneticField {
    //Electric field pushes a charged particle along or against this direction.
    glm::vec3 electric = glm::vec3(0.12f, 0.0f, 0.0f);

    //Magnetic field turns a moving charged particle sideways.
    glm::vec3 magnetic = glm::vec3(-0.20f, 0.46f, -0.18f);
};
