# Electron Field Simulator

A C++ and OpenGL project for visualizing charged particle motion in electromagnetic fields.

## Current progress

- GLFW window and OpenGL context
- GLEW shader loading
- First coloured triangle rendered with OpenGL

## Planned features

- 3D camera and coordinate grid
- Electron and nucleus rendering
- Electric and magnetic field controls
- Lorentz force simulation and trajectory trail

## Build in WSL

```bash
cmake -S . -B build
cmake --build build
./build/Electron_Simulation
```
