# Electron Field Simulator

![Build](https://github.com/parniayzdin/Electron_Simulation/actions/workflows/build.yml/badge.svg)

Electron Field Simulator is a C++17 and OpenGL visualization project that combines a classical charged-particle simulation with a quantum-mechanical hydrogen orbital visualization. The application renders a three-dimensional scene in real time, integrates electron motion through uniform electric and magnetic fields, records the resulting trajectory, and displays a sampled hydrogen probability distribution as a GPU-rendered point cloud.

The project is intended as both a graphics application and a physics demonstration. The classical particle simulation and the quantum orbital visualization represent different physical models: the trajectory view treats an electron as a charged particle moving through prescribed electromagnetic fields, while the orbital view represents spatial probability density derived from a hydrogenic wavefunction. The two views should therefore not be interpreted as one simultaneous physical model of the same electron.

## Physical model

### Charged-particle motion

The classical simulation begins with the Lorentz force law

$$
\mathbf{F}=q\left(\mathbf{E}+\mathbf{v}\times\mathbf{B}\right),
$$

where $q$ is particle charge, $\mathbf{E}$ is the electric field, $\mathbf{v}$ is velocity, and $\mathbf{B}$ is the magnetic field. Using $\mathbf{F}=m\mathbf{a}$ gives the equation integrated by the simulator:

$$
\frac{d\mathbf{v}}{dt}=\frac{q}{m}\left(\mathbf{E}+\mathbf{v}\times\mathbf{B}\right).
$$

The current implementation uses normalized simulation units rather than SI electron constants. The particle definition uses $q=-1$ and $m=1$, so the dynamics are governed directly by the selected field vectors and timestep. The default magnetic field is uniform and tilted in three dimensions, while the default electric field is zero.

When $\mathbf{E}=0$ and a particle has velocity components both parallel and perpendicular to a uniform magnetic field, the expected motion is helical. The characteristic cyclotron angular frequency is

$$
\omega_c=\frac{|q|B}{m},
$$

and the Larmor radius is

$$
r_L=\frac{m v_\perp}{|q|B}.
$$

The component $v_\parallel$ is unaffected by the magnetic force, so it carries the electron along the field direction while $v_\perp$ produces circular motion around that axis. The pitch of the ideal helix is

$$
p=\frac{2\pi v_\parallel}{\omega_c}.
$$

The application constructs an initial state consistent with this geometry and then computes the trajectory numerically rather than drawing a precomputed helix.

## Boris particle integrator

Particle motion is advanced with the Boris method, a standard integrator for charged-particle dynamics in electromagnetic fields. It separates the electric acceleration from the magnetic rotation, which gives substantially better long-term behavior for magnetic orbits than a simple forward-Euler update.

For timestep $\Delta t$, the first half electric-field acceleration is

$$
\mathbf{v}^{-}=\mathbf{v}^{n}+\frac{q\Delta t}{2m}\mathbf{E}.
$$

The magnetic rotation is then constructed from

$$
\mathbf{t}=\frac{q\Delta t}{2m}\mathbf{B},
\qquad
\mathbf{s}=\frac{2\mathbf{t}}{1+|\mathbf{t}|^2}.
$$

The intermediate and rotated velocities are

$$
\mathbf{v}'=\mathbf{v}^{-}+\mathbf{v}^{-}\times\mathbf{t},
$$

$$
\mathbf{v}^{+}=\mathbf{v}^{-}+\mathbf{v}'\times\mathbf{s}.
$$

A second half electric-field acceleration completes the step:

$$
\mathbf{v}^{n+1}=\mathbf{v}^{+}+\frac{q\Delta t}{2m}\mathbf{E},
$$

followed by the position update

$$
\mathbf{x}^{n+1}=\mathbf{x}^{n}+\mathbf{v}^{n+1}\Delta t.
$$

This algorithm is implemented in `src/Physics.cpp`. Recent particle positions are retained and uploaded to OpenGL as a trajectory trail, allowing the numerical path to be inspected visually.

## Hydrogen orbital visualization

The atom overview visualizes a hydrogenic orbital using the quantum numbers

$$
n=4,\qquad \ell=3,\qquad m=2.
$$

For hydrogen-like stationary states, the spatial wavefunction separates into radial and angular components:

$$
\psi_{n\ell m}(r,\theta,\phi)=R_{n\ell}(r)Y_{\ell}^{m}(\theta,\phi).
$$

The rendered quantity is based on probability density,

$$
P(r,\theta,\phi)\propto |\psi_{n\ell m}(r,\theta,\phi)|^2.
$$

The radial factor implemented in `src/AtomOverview.cpp` has the hydrogenic form

$$
R_{n\ell}(r)\propto e^{-\rho/2}\rho^{\ell}L_{n-\ell-1}^{2\ell+1}(\rho),
\qquad
\rho=\frac{2r}{n},
$$

using normalized length units. The angular structure uses an associated Legendre polynomial together with a real azimuthal component:

$$
Y(\theta,\phi)\propto P_{\ell}^{|m|}(\cos\theta)\cos(m\phi).
$$

The program evaluates these factors numerically, builds cumulative probability distributions, and samples 65,000 three-dimensional positions. In spherical coordinates, sampled points are converted to Cartesian coordinates through

$$
x=r\sin\theta\cos\phi,
\qquad
y=r\cos\theta,
\qquad
z=r\sin\theta\sin\phi.
$$

Regions with greater calculated density receive more points. Point color is a visualization mapping of relative density and is not itself a physical observable. Three reference planes are also rendered to make the orbital geometry easier to interpret from different camera angles.

## Rendering architecture

The active application target is built from `main.cpp`, `AtomOverview.cpp`, `Physics.cpp`, `Shader.cpp`, and `Sphere.cpp`. OpenGL performs the real-time rendering, GLFW creates the window and handles mouse/input events, GLEW loads OpenGL functions, GLM provides vector and matrix operations, and Dear ImGui provides the application overlay.

The scene contains a three-dimensional coordinate grid, a rendered electron sphere, a magnetic-field reference axis, the dynamically updated electron trail, and the hydrogen probability cloud. An orbit camera converts yaw, pitch, and distance into a view matrix with `glm::lookAt`. Mouse dragging changes the viewing direction and the scroll wheel changes camera distance. Vertex and fragment shaders in `shaders/` handle the GPU rendering stages.

The orbital point cloud is uploaded as static geometry because its probability samples do not change every frame. The electron trail is dynamic geometry: recent positions are collected on the CPU and its GPU buffer is updated as the simulation advances. This separates expensive static sampling from the smaller per-frame trajectory update.

## Project structure

```text
Electron_Simulation/
├── .github/workflows/build.yml   # Continuous-integration CMake build
├── CMakeLists.txt                # C++17 build configuration
├── shaders/                      # GLSL vertex and fragment shaders
└── src/
    ├── main.cpp                  # Window, rendering loop, camera, scene setup
    ├── Physics.cpp/.hpp          # Boris charged-particle integration
    ├── Particle.hpp              # Particle state and trajectory history
    ├── ElectromagneticField.hpp  # Uniform electric and magnetic fields
    ├── AtomOverview.cpp/.hpp     # Hydrogen orbital density and sampling
    ├── Shader.cpp/.hpp           # GLSL program loading and management
    └── Sphere.cpp/.hpp           # Sphere mesh generation and rendering
```

## Build requirements

The project uses CMake and requires a C++17 compiler together with OpenGL development libraries. On Ubuntu or WSL, install the required packages with:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  pkg-config \
  libglfw3-dev \
  libglew-dev \
  libglm-dev \
  libgl1-mesa-dev
```

Dear ImGui is downloaded automatically by CMake through `FetchContent`.

Configure and build the application with:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Run it from the repository root with:

```bash
./build/Electron_Simulation
```

Running from the repository root or from the generated build directory ensures the copied shader files can be found by the executable.

## Continuous integration

Every push to `main` and every pull request targeting `main` runs the GitHub Actions build workflow. The workflow installs the Linux OpenGL development dependencies, configures the project with CMake, and compiles the full `Electron_Simulation` target. A successful workflow produces the green GitHub check associated with the commit and updates the build badge at the top of this README.

The CI job verifies compilation and dependency integration. It does not launch the graphical application because GitHub's hosted runner does not provide the interactive display environment used by the simulator.
