#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

struct OrbitCamera {
    static constexpr float minimumDistance = 0.28f;
    static constexpr float maximumDistance = 24.0f;
    float yawDegrees = 32.0f;
    float pitchDegrees = 20.0f;
    float distance = 12.0f;
    float requestedDistance = 12.0f;
    bool isDragging = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

    void zoom(double scroll) {
        if (!std::isfinite(scroll)) return;
        setDistance(requestedDistance * std::exp(-0.12f *
            static_cast<float>(std::clamp(scroll, -100.0, 100.0))));
    }

    void setDistance(float value) {
        if (std::isfinite(value))
            requestedDistance = std::clamp(value, minimumDistance, maximumDistance);
    }

    void update(float dt) {
        distance += (requestedDistance - distance) *
            (1.0f - std::exp(-12.0f * std::max(dt, 0.0f)));
        if (std::abs(requestedDistance - distance) < 0.0001f)
            distance = requestedDistance;
    }

    glm::mat4 viewMatrix(const glm::vec3& target) const {
        const float yaw = glm::radians(yawDegrees);
        const float pitch = glm::radians(pitchDegrees);
        const glm::vec3 position = target + distance * glm::vec3(
            std::cos(pitch) * std::sin(yaw), std::sin(pitch),
            std::cos(pitch) * std::cos(yaw));
        return glm::lookAt(position, target, glm::vec3(0, 1, 0));
    }
};
