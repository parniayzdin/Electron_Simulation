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
    AtomOverview();

    //Returns the coloured points that form the orbital probability cloud.
    const std::vector<Vertex>& orbitalVertices() const;

    //Returns the three white guide planes shown around the orbital.
    const std::vector<Vertex>& clippingPlaneVertices() const;

private:
    std::vector<Vertex> orbitalVertices_;
    std::vector<Vertex> clippingPlaneVertices_;
};
