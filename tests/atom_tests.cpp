#include "AtomOverview.hpp"
#include "LearnedDensity.hpp"
#include "OrbitCamera.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void sampling() {
    for (int n = 1; n <= 4; ++n) for (int l = 0; l < n; ++l)
        for (int m = 0; m <= l; ++m) {
            AtomOverview atom(n, l, m);
            double radiusSum = 0, yAngularSum = 0;
            glm::dvec3 center(0);
            for (const auto& vertex : atom.orbitalVertices()) {
                glm::dvec3 p(vertex.position[0], vertex.position[1], vertex.position[2]);
                const double r = glm::length(p);
                require(std::isfinite(r), "Non-finite cloud sample");
                for (float c : vertex.color)
                    require(std::isfinite(c) && c >= 0 && c <= 1, "Invalid dot color");
                radiusSum += r / AtomOverview::worldScale;
                center += p;
                if (r > 0) yAngularSum += p.y * p.y / (r * r);
            }
            const double count = static_cast<double>(atom.orbitalVertices().size());
            require(count == 40000, "Missing orbital samples");
            const double expectedRadius = 0.5 * (3 * n * n - l * (l + 1));
            require(std::abs(radiusSum / count - expectedRadius) < expectedRadius * 0.025,
                "Sampled radial mean does not match analytic hydrogen expectation");
            require(glm::length(center / count) < expectedRadius * AtomOverview::worldScale * 0.015,
                "Orbital is not centered on the nucleus");
            if (n == 1)
                require(std::abs(yAngularSum / count - 1.0 / 3.0) < 0.01,
                    "1s cloud is not isotropic");
            if (n == 2 && l == 1 && m == 0)
                require(std::abs(yAngularSum / count - 0.6) < 0.01,
                    "2p lobes do not follow the expected angular distribution");
            require(atom.nucleusVertices().size() == 2400, "Proton marker missing");
            for (const auto& v : atom.nucleusVertices()) {
                const float r = glm::length(glm::vec3(v.position[0], v.position[1], v.position[2]));
                require(std::abs(r - AtomOverview::protonDisplayRadius) < 1e-6,
                    "Proton marker radius changed");
            }
        }
    bool rejected = false;
    try { AtomOverview invalid(1, 1, 0); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "Invalid quantum numbers accepted");
}

void zoom() {
    OrbitCamera camera;
    camera.zoom(2);
    camera.zoom(-2);
    require(std::abs(camera.requestedDistance - 12) < 1e-5, "Scroll round trip drift");
    camera.zoom(10000);
    require(camera.requestedDistance == OrbitCamera::minimumDistance, "Unsafe minimum zoom");
    camera.zoom(-10000);
    require(camera.requestedDistance == OrbitCamera::maximumDistance, "Unbounded zoom out");
    camera.setDistance(std::numeric_limits<float>::quiet_NaN());
    require(std::isfinite(camera.requestedDistance), "Non-finite camera target accepted");

    OrbitCamera slow, fast;
    slow.setDistance(0.28f);
    fast.setDistance(0.28f);
    for (int i = 0; i < 30; ++i) slow.update(1.0f / 30);
    for (int i = 0; i < 144; ++i) fast.update(1.0f / 144);
    require(std::abs(slow.distance - fast.distance) < 0.0002,
        "Zoom smoothing depends on display refresh rate");
    for (float distance : {24.0f, 12.0f, 10.01f, 10.0f, 9.99f, 3.0f, 0.28f}) {
        camera.distance = distance;
        for (float pitch : {-89.0f, 0.0f, 89.0f}) {
            camera.pitchDegrees = pitch;
            const auto view = camera.viewMatrix(glm::vec3(0));
            for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
                require(std::isfinite(view[i][j]), "Invalid close-up camera matrix");
        }
    }
}

void learnedDensity() {
    double absoluteError = 0, exactMass = 0;
    for (int i = 0; i < 2400; ++i) {
        const float r = 16.0f * (i + 0.5f) / 2400;
        const float predicted = learnedGroundStateDensity(r);
        const double exact = std::exp(-2.0 * r);
        require(std::isfinite(predicted) && predicted >= 0, "Invalid learned density");
        require(std::abs(predicted - exact) < 0.01, "Learned pointwise error too large");
        absoluteError += r * r * std::abs(predicted - exact);
        exactMass += r * r * exact;
    }
    require(absoluteError / exactMass < 0.01, "Learned radial distribution error exceeds 1 percent");
    require(learnedGroundStateDensity(-1) == 0 && learnedGroundStateDensity(20) == 0 &&
        learnedGroundStateDensity(std::numeric_limits<float>::quiet_NaN()) == 0,
        "Invalid model input was not rejected");
    AtomOverview atom(1, 0, 0, true);
    double radius = 0, angular = 0;
    for (const auto& v : atom.orbitalVertices()) {
        glm::dvec3 p(v.position[0], v.position[1], v.position[2]);
        const double r = glm::length(p);
        require(std::isfinite(r), "Invalid learned point");
        radius += r / AtomOverview::worldScale;
        if (r > 0) angular += p.y * p.y / (r * r);
        for (float c : v.color) require(std::isfinite(c), "Invalid learned color");
    }
    require(atom.orbitalVertices().size() == 40000, "Missing learned samples");
    require(std::abs(radius / 40000 - 1.5) < 0.025, "Learned cloud radial mean drifted");
    require(std::abs(angular / 40000 - 1.0 / 3) < 0.01, "Learned cloud lost isotropy");
    bool rejected = false;
    try { AtomOverview invalid(2, 0, 0, true); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "Unsupported learned orbital accepted");
    std::cout << "Learned radial weighted L1 error: " << absoluteError / exactMass * 100 << "%\n";
}

int main() {
    try { sampling(); zoom(); learnedDensity(); }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
    std::cout << "PASS: 20 hydrogen orbitals, analytic moments, proton geometry, zoom bounds and smoothing.\n";
}
