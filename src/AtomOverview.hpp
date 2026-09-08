#pragma once

#include <vector>

//A position and colour that can be sent to the GPU.
struct Vertex {
    float position[3];
    float color[3];
};

//Builds a fixed hydrogen-orbital probability view without creating a window.
class AtomOverview {
public:
    AtomOverview(int n = 4, int l = 3, int m = 2);

    static constexpr float worldScale = 0.16f; // World units per Bohr radius.
    static constexpr float protonDisplayRadius = 0.065f; // Enlarged visual marker.
    float overviewDistance() const;
    const std::vector<Vertex>& nucleusVertices() const;

    //Returns the coloured points that form the orbital probability cloud.
    const std::vector<Vertex>& orbitalVertices() const;

    //Returns the three white guide planes shown around the orbital.
    const std::vector<Vertex>& clippingPlaneVertices() const;

private:
    std::vector<Vertex> orbitalVertices_;
    std::vector<Vertex> clippingPlaneVertices_;
    std::vector<Vertex> nucleusVertices_;
    int n_;
};
