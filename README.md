# Hydrogen Atom Explorer

A C++17 / OpenGL visualization of hydrogen. Zoom from the original orbital overview into the same atom, with small circular cloud dots and a dotted proton at the center. Zooming no longer switches to the magnetic-field scene.

## Run

On Windows with Ubuntu / WSLg, double-click **Launch Hydrogen Atom.cmd**. It builds and starts the application using your existing Ubuntu installation.

For a fresh installation, install the dependencies in Ubuntu first:

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libglfw3-dev libglew-dev libglm-dev libgl1-mesa-dev
```

Or build and run directly in Ubuntu / WSL:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
./build/Electron_Simulation
```

Requires CMake 3.24+, a C++17 compiler, and OpenGL 3.3. Dear ImGui v1.91.6 downloads automatically during the first build. Shaders are copied beside the executable.

## Controls

- **Scroll:** smoothly zoom in or out.
- **Left drag:** orbit the atom.
- **Whole atom / H:** return to the overview.
- **Inside cloud:** inspect the same probability cloud from closer up.
- **Nucleus close-up / N:** focus on the dotted proton.
- **Cutaway:** reveal the interior by removing the +X side of the cloud.
- **Space / Slow orbit:** toggle gentle camera rotation.
- **Brightness / Dot size:** adjust the circular dots.
- **Esc:** close.

The orbital picker offers 1s, 2s, 2p, two 3d views, and the original 4f cloud. Zooming preserves the selected orbital. The panel scrolls on smaller windows.

## What you are seeing

Hydrogen has **one proton and one electron**. The 180,000 cloud dots sample possible locations of that one electron; they are not separate electrons or a trajectory. Their density follows the squared hydrogen wavefunction, including the spherical volume element when sampling. A fixed random seed keeps the cloud stable while you explore it.

For nonzero |m|, the displayed shape is a real cosine combination of the +m and -m states. The Y axis is the polar axis. Color and brightness help visualize the density; they are not quantitative measurements.

The proton is **enlarged for visibility**. Its 2,400 dots form a visual texture, not individual physical particles or quarks. Camera distances use the cloud scale of 0.16 world units per Bohr radius (a0), while the proton marker has an illustrative radius of 0.065 world units.

Higher orbitals such as 4f have very little electron probability near the nucleus, so empty regions are expected. Choose 1s for a cloud concentrated near the center. Slow orbit moves only the camera; the probability distribution stays stationary.

Background: [OpenStax: The Hydrogen Atom](https://openstax.org/books/university-physics-volume-3/pages/8-2-the-hydrogen-atom).

## Checks

```bash
ctest --test-dir build --output-on-failure
./build/Electron_Simulation --smoke-test build/captures
```

Tests check all 20 supported real cosine states through n=4, analytic mean radius, 1s isotropy, 2p angular distribution, finite geometry, and camera zoom bounds and smoothing. The render check captures six views, including the former scene-switch distance, interior, proton, and minimum zoom, and fails on OpenGL errors. Use `xvfb-run -a` before the executable on headless Linux. GitHub Actions runs both checks.

The active app uses `main.cpp`, `AtomOverview.cpp`, `OrbitCamera.hpp`, `Shader.cpp`, and the atom shaders. Earlier classical physics modules and standalone experiments remain in the repository for reference.
