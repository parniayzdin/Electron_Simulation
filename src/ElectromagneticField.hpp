#pragma once

#include <glm/glm.hpp>

//Stores the two uniform fields used by the whole simulation scene.
struct ElectromagneticField {
    //Electric field pushes a charged particle along or against this direction.
    //Zero electric field keeps this demonstration as a clean magnetic helix.
    glm::vec3 electric = glm::vec3(0.0f, 0.0f, 0.0f);

    //Magnetic field turns a moving charged particle sideways.
    //The larger magnitude makes each visible orbit complete quickly.
    glm::vec3 magnetic = glm::vec3(-1.00f, 1.00f, 0.50f);
};
