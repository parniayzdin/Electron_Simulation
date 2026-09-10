<div align="center">
  <h1>Hydrogen Atom Explorer</h1>
</div>

I built this project to turn the hydrogen atom from something I had mostly seen as equations into something I could actually explore. It is a C++17 and OpenGL visualization that renders a 40,000 point probability cloud and lets me move from an orbital overview into the same atom, inspect the cloud from inside, and zoom toward the proton.

## Demo

<p align="center">
  <img src="assets/demo.gif" alt="Hydrogen Atom Explorer Demo" width="900">
</p>

## What I built

I represent the electron as a probability cloud rather than as a particle travelling along a fixed orbit. The 40,000 dots are sampled possible locations of the same electron, so denser regions correspond to locations where the electron is more likely to be found. I also added multiple hydrogen orbitals, camera controls, close up views, a cutaway mode, adjustable point size and brightness, and a Dear ImGui control panel. I enlarged the proton for visibility so the nucleus remains easy to inspect while zooming.

## Physics behind the visualization

The main idea I use is that the electron is described by a wavefunction, usually written as:

$$
\psi(r,\theta,\phi)
$$

I do not draw the wavefunction directly. Instead, I use its squared magnitude to represent where the electron is more likely to be found:

$$
|\psi|^2
$$

That probability is what drives the shape of the cloud. Regions with higher probability receive more points, which is why the orbital appears denser in some areas and nearly empty in others. I sample 40,000 three dimensional points from that probability distribution and convert them into positions OpenGL can render. Different orbitals change the probability pattern, which produces the different cloud shapes shown in the application.

## Rendering

I upload the sampled points to OpenGL buffers and render the cloud on the GPU. GLFW manages the window and OpenGL context, GLEW loads the OpenGL functions, GLM handles camera and matrix math, and Dear ImGui provides the interactive controls.

I use a fixed random seed so the generated cloud stays stable while I move the camera. Zooming and orbiting therefore change the viewpoint without regenerating the probability distribution.

The proton uses 2,400 dots as a visual texture. Those dots are not individual physical particles or quarks, and the proton marker is intentionally enlarged so it remains visible beside the orbital cloud.

## Controls

- **Scroll:** zoom in or out
- **Left drag:** orbit around the atom
- **Whole atom / H:** return to the orbital overview
- **Inside cloud:** move into the probability cloud
- **Nucleus close up / N:** focus on the proton
- **Cutaway:** remove the positive X side of the cloud to reveal the interior
- **Space / Slow orbit:** toggle automatic camera rotation
- **Brightness / Dot size:** adjust the rendered cloud
- **Esc:** close the application

The orbital picker includes 1s, 2s, 2p, two 3d views, and the original 4f cloud. Zooming keeps the selected orbital active.

## Run

On Windows with Ubuntu / WSLg, double click **Launch Hydrogen Atom.cmd**. It builds and starts the application using the existing Ubuntu installation.

For a fresh Ubuntu installation:

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libglfw3-dev libglew-dev libglm-dev libgl1-mesa-dev
```

Then build and run:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
./build/Electron_Simulation
```

The project requires CMake 3.24+, a C++17 compiler, and OpenGL 3.3. Dear ImGui v1.91.6 downloads automatically during the first build.

## Learned density experiment

Select **1s / ground state**, then enable **Use learned density** to render the
cloud using a small trained regression model. Disable it to compare with the
exact formula. Other orbitals keep their exact formulas and disable the toggle.

The model learns the ground state radial density from 601 synthetic examples
between 0 and 12 Bohr radii. It uses 27 fixed Gaussian basis functions and fits
their output weights with ridge regression. Training fits the square root of
density; C++ squares the output so predictions cannot become negative. The
radial sampler still applies the spherical volume factor r squared.

The weights are included in the project, so the app needs no Python process,
model download, or additional C++ library. This is a supervised approximation
of a known formula for learning purposes, not a faster renderer or a solver
for new orbitals. The negligible density tail beyond 12 Bohr radii is truncated.

To retrain with Python 3 and its standard library:

```bash
python3 ml/train_model.py
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

The trainer generates labels directly, validates on a separate midpoint grid,
and exports `src/LearnedDensityWeights.hpp`. Run
`python3 ml/generate_training_data.py` to inspect those training labels as CSV.
Run `python3 ml/train_model.py --check` to validate without changing the weights.

The initial model has approximately **0.44% volume-weighted relative L1 density
error** on held-out radii. This measures approximation error, not rendering
performance. C++ tests independently require less than 1% distribution error
and check the sampled mean radius, isotropy, finite geometry, and unsupported
orbital handling. Rendering smoke tests capture both learned and exact 1s views.

## Testing

I added automated checks for all 20 supported real cosine states through n=4, analytic mean radius, 1s isotropy, 2p angular distribution, finite geometry, and camera zoom behavior.

```bash
ctest --test-dir build --output-on-failure
./build/Electron_Simulation --smoke-test build/captures
```

I also use GitHub Actions to run the automated checks in CI.

## Notes

Hydrogen has one proton and one electron. The cloud dots are not separate electrons and do not represent a trajectory. Higher orbitals such as 4f naturally have very little probability near the nucleus, while 1s is much more concentrated near the center.

For more background on the physics, I used [OpenStax: The Hydrogen Atom](https://openstax.org/books/university-physics-volume-3/pages/8-1-the-hydrogen-atom) as a reference.
