#pragma once
#include "LearnedDensityWeights.hpp"
#include <cmath>

// Approximate 1s density only. Truncate the negligible tail outside training.
inline float learnedGroundStateDensity(float radius) {
    if (!std::isfinite(radius) || radius < 0 || radius > learned_density::maximumRadius)
        return 0.0f;
    double amplitude = 0;
    for (std::size_t i = 0; i < learned_density::weights.size(); ++i) {
        const double x = (radius - learned_density::centers[i]) / learned_density::width;
        amplitude += learned_density::weights[i] * std::exp(-x * x);
    }
    return static_cast<float>(amplitude * amplitude);
}
