#include "Physics.hpp"

#include <glm/glm.hpp>

void advanceParticleWithBoris(
    Particle& particle,
    const ElectromagneticField& field,
    float timeStep
) {
    //Charge divided by mass decides how strongly this particle reacts.
    const float chargeToMass = particle.charge / particle.mass;

    //The electric field changes velocity before the magnetic turn.
    const glm::vec3 halfElectricKick =
        field.electric * (chargeToMass * timeStep * 0.5f);

    const glm::vec3 velocityMinus =
        particle.velocity + halfElectricKick;

    //These two values rotate the velocity around the magnetic-field direction.
    const glm::vec3 t =
        field.magnetic * (chargeToMass * timeStep * 0.5f);

    const glm::vec3 s =
        (2.0f * t) / (1.0f + glm::dot(t, t));

    const glm::vec3 velocityPrime =
        velocityMinus + glm::cross(velocityMinus, t);

    const glm::vec3 velocityPlus =
        velocityMinus + glm::cross(velocityPrime, s);

    //The second half of the electric-field change completes the Boris step.
    particle.velocity = velocityPlus + halfElectricKick;

    //The new velocity tells us where the particle is at the next frame.
    particle.position += particle.velocity * timeStep;
}
