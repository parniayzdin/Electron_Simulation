#pragma once

#include "ElectromagneticField.hpp"
#include "Particle.hpp"

//Moves one charged particle forward by one short physics time step.
void advanceParticleWithBoris(
    Particle& particle,
    const ElectromagneticField& field,
    float timeStep
);
