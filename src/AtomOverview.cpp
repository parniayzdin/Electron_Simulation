#include "AtomOverview.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>
#include <vector>

namespace {

constexpr std::size_t POINT_COUNT = 40000;
constexpr float PI = 3.14159265f;

float associatedLaguerre(int order, int alpha, float value)
{
    if (order == 0) {
        return 1.0f;
    }

    float previous = 1.0f;
    float current = 1.0f + alpha - value;

    for (int index = 2; index <= order; ++index) {
        const float next = (
            (2.0f * index - 1.0f + alpha - value) * current -
            (index - 1.0f + alpha) * previous
        ) / index;
        previous = current;
        current = next;
    }

    return current;
}

float associatedLegendre(int degree, int order, float value)
{
    float pmm = 1.0f;

    if (order > 0) {
        const float sinTheta = std::sqrt(
            std::max(0.0f, 1.0f - value * value)
        );
        float factor = 1.0f;

        for (int index = 1; index <= order; ++index) {
            pmm *= -factor * sinTheta;
            factor += 2.0f;
        }
    }

    if (degree == order) {
        return pmm;
    }

    float pmmp1 = value * (2.0f * order + 1.0f) * pmm;
    if (degree == order + 1) {
        return pmmp1;
    }

    for (int index = order + 2; index <= degree; ++index) {
        const float pll = (
            (2.0f * index - 1.0f) * value * pmmp1 -
            (index + order - 1.0f) * pmm
        ) / (index - order);
        pmm = pmmp1;
        pmmp1 = pll;
    }

    return pmmp1;
}

float radialDensity(int n, int l, float radius)
{
    const float rho = 2.0f * radius / n;
    const float wave = std::exp(-rho * 0.5f) *
        std::pow(rho, static_cast<float>(l)) *
        associatedLaguerre(n - l - 1, 2 * l + 1, rho);
    return wave * wave;
}

float angularDensity(int l, int m, float theta, float phi)
{
    const float wave = associatedLegendre(l, std::abs(m), std::cos(theta)) *
        std::cos(m * phi);
    return wave * wave;
}

std::vector<float> buildCdf(const std::vector<float>& weights)
{
    std::vector<float> cdf(weights.size());
    float sum = 0.0f;

    for (std::size_t index = 0; index < weights.size(); ++index) {
        sum += weights[index];
        cdf[index] = sum;
    }

    if (!(sum > 0.0f) || !std::isfinite(sum))
        throw std::runtime_error("Invalid orbital probability distribution.");

    for (float& value : cdf) {
        value /= sum;
    }

    return cdf;
}

float sampleCdf(
    const std::vector<float>& cdf,
    float maximumValue,
    std::mt19937& generator
) {
    std::uniform_real_distribution<float> randomValue(0.0f, 1.0f);
    const float target = randomValue(generator);
    const auto iterator = std::lower_bound(
        cdf.begin(),
        cdf.end(),
        target
    );
    const std::size_t index = std::min(static_cast<std::size_t>(
        std::distance(cdf.begin(), iterator)
    ), cdf.size() - 1);
    if (index == 0) return 0.0f;
    const float previous = cdf[index - 1];
    const float weight = cdf[index] - previous;
    const float fraction = weight > 0.0f ? (target - previous) / weight : 0.0f;
    return maximumValue * (static_cast<float>(index - 1) + fraction) /
        static_cast<float>(cdf.size() - 1);
}

glm::vec3 fireColor(float strength)
{
    strength = std::clamp(strength, 0.0f, 1.0f);
    const glm::vec3 purple(0.35f, 0.00f, 0.70f);
    const glm::vec3 magenta(0.95f, 0.00f, 0.80f);
    const glm::vec3 orange(1.00f, 0.30f, 0.03f);
    const glm::vec3 yellow(1.00f, 0.92f, 0.18f);

    if (strength < 0.35f) {
        return purple * (strength / 0.35f);
    }
    if (strength < 0.62f) {
        return glm::mix(
            purple,
            magenta,
            (strength - 0.35f) / 0.27f
        );
    }
    if (strength < 0.84f) {
        return glm::mix(
            magenta,
            orange,
            (strength - 0.62f) / 0.22f
        );
    }

    return glm::mix(
        orange,
        yellow,
        (strength - 0.84f) / 0.16f
    );
}

void addLine(
    std::vector<Vertex>& vertices,
    const glm::vec3& start,
    const glm::vec3& end,
    const glm::vec3& color
) {
    vertices.push_back({
        {start.x, start.y, start.z},
        {color.r, color.g, color.b}
    });
    vertices.push_back({
        {end.x, end.y, end.z},
        {color.r, color.g, color.b}
    });
}

void addPlaneOutline(
    std::vector<Vertex>& vertices,
    const glm::vec3& firstAxis,
    const glm::vec3& secondAxis
) {
    constexpr float HALF_WIDTH = 4.8f;
    const glm::vec3 color(0.88f, 0.88f, 0.92f);
    const glm::vec3 first = firstAxis * HALF_WIDTH;
    const glm::vec3 second = secondAxis * HALF_WIDTH;
    const glm::vec3 topLeft = -first + second;
    const glm::vec3 topRight = first + second;
    const glm::vec3 bottomRight = first - second;
    const glm::vec3 bottomLeft = -first - second;

    addLine(vertices, topLeft, topRight, color);
    addLine(vertices, topRight, bottomRight, color);
    addLine(vertices, bottomRight, bottomLeft, color);
    addLine(vertices, bottomLeft, topLeft, color);
}

} //namespace

AtomOverview::AtomOverview(int n, int l, int m) : n_(n)
{
    if (n < 1 || n > 4 || l < 0 || l >= n || m < 0 || m > l)
        throw std::invalid_argument("Expected 1 <= n <= 4, 0 <= l < n, 0 <= m <= l.");
    const float maxSampleRadius = 4.0f * n * n + 12.0f;
    constexpr int RADIAL_SAMPLES = 2048;
    constexpr int THETA_SAMPLES = 1024;
    constexpr int PHI_SAMPLES = 720;

    std::vector<float> radialWeights(RADIAL_SAMPLES);
    std::vector<float> thetaWeights(THETA_SAMPLES);
    std::vector<float> phiWeights(PHI_SAMPLES);

    for (int index = 0; index < RADIAL_SAMPLES; ++index) {
        const float radius = maxSampleRadius * index /
            static_cast<float>(RADIAL_SAMPLES - 1);
        const float radialPart = radialDensity(n, l, radius);
        radialWeights[index] = radius * radius * radialPart;
    }

    for (int index = 0; index < THETA_SAMPLES; ++index) {
        const float theta = PI * index /
            static_cast<float>(THETA_SAMPLES - 1);
        const float angularPart = angularDensity(l, m, theta, 0.0f);
        thetaWeights[index] = std::sin(theta) * angularPart;
    }

    for (int index = 0; index < PHI_SAMPLES; ++index) {
        const float phi = 2.0f * PI * index /
            static_cast<float>(PHI_SAMPLES - 1);
        phiWeights[index] = std::cos(m * phi) *
            std::cos(m * phi);
    }

    const std::vector<float> radialCdf = buildCdf(radialWeights);
    const std::vector<float> thetaCdf = buildCdf(thetaWeights);
    const std::vector<float> phiCdf = buildCdf(phiWeights);

    std::mt19937 generator(17);
    std::vector<glm::vec3> positions;
    std::vector<float> densities;
    positions.reserve(POINT_COUNT);
    densities.reserve(POINT_COUNT);
    float maximumDensity = 0.0f;

    for (std::size_t index = 0; index < POINT_COUNT; ++index) {
        const float radius = sampleCdf(
            radialCdf,
            maxSampleRadius,
            generator
        );
        const float theta = sampleCdf(thetaCdf, PI, generator);
        const float phi = sampleCdf(phiCdf, 2.0f * PI, generator);
        const float density = radialDensity(n, l, radius) * angularDensity(l, m, theta, phi);
        const float sinTheta = std::sin(theta);

        positions.push_back(worldScale * radius * glm::vec3(
            sinTheta * std::cos(phi),
            std::cos(theta),
            sinTheta * std::sin(phi)
        ));
        densities.push_back(density);
        maximumDensity = std::max(maximumDensity, density);
    }

    orbitalVertices_.reserve(POINT_COUNT);
    for (std::size_t index = 0; index < positions.size(); ++index) {
        const float strength = std::pow(
            densities[index] / maximumDensity,
            0.28f
        );
        const glm::vec3 color = fireColor(strength);
        const glm::vec3& position = positions[index];

        orbitalVertices_.push_back({
            {position.x, position.y, position.z},
            {color.r, color.g, color.b}
        });
    }

    // Dots form one proton marker, not individual nucleons or quarks.
    constexpr int nucleusPoints = 2400;
    constexpr float goldenAngle = 2.39996323f;
    for (int i = 0; i < nucleusPoints; ++i) {
        const float y = 1.0f - 2.0f * (i + 0.5f) / nucleusPoints;
        const float ring = std::sqrt(1.0f - y * y);
        const float angle = i * goldenAngle;
        const glm::vec3 p = protonDisplayRadius *
            glm::vec3(ring * std::cos(angle), y, ring * std::sin(angle));
        const float light = 0.55f + 0.45f * (y + 1.0f) * 0.5f;
        nucleusVertices_.push_back({
            {p.x, p.y, p.z}, {1.0f, 0.22f + 0.42f * light, 0.12f}});
    }

    //Three intersecting planes create the clipped-orbital framing.
    addPlaneOutline(
        clippingPlaneVertices_,
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    addPlaneOutline(
        clippingPlaneVertices_,
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    addPlaneOutline(
        clippingPlaneVertices_,
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
}

const std::vector<Vertex>& AtomOverview::orbitalVertices() const
{
    return orbitalVertices_;
}

const std::vector<Vertex>& AtomOverview::clippingPlaneVertices() const
{
    return clippingPlaneVertices_;
}

float AtomOverview::overviewDistance() const
{
    return std::max(1.8f, 0.75f * n_ * n_);
}

const std::vector<Vertex>& AtomOverview::nucleusVertices() const
{
    return nucleusVertices_;
}
