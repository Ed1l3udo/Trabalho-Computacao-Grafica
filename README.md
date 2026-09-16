# Computer Graphics Ray Tracer

![Offline render of the scene](docs/images/render.png)

A C++17 computer graphics project created for a Computer Graphics course at UFC. It implements its own ray-based offline renderer and an interactive preview of the same scene. Raylib is used for the preview window, input, and presentation layer; it does not perform the ray tracing calculations.

## Features

- Offline rendering to a PPM image.
- Interactive Raylib preview with free-flight camera and object picking/selection.
- Perspective, orthographic, and oblique camera projections, plus one-, two-, and three-point perspective presets.
- Point, directional, spotlight, and ambient lights with shadows.
- Ambient, diffuse, and specular material terms.
- Spheres, boxes, cylinders, cones, and triangular OBJ meshes.
- OBJ position/face loading, including fan triangulation for polygonal faces.
- PPM texture sampling.
- Object transforms and inverse-transpose normal transforms.

## How it works

For each pixel, the camera generates a ray and the renderer selects the closest valid intersection among the scene objects. Intersections are evaluated in object space and transformed back to world space. Surface normals use the inverse-transpose transform, then ambient, diffuse, and specular terms are evaluated with the selected light. Shadow rays test whether a light is occluded.

Meshes are loaded from OBJ files with vertex positions and faces. Polygonal faces are converted to triangles by fan triangulation. The loader reports malformed assets with their path and, when applicable, line number.

## Run modes

### Offline renderer

The offline program renders the scene to `saida.ppm` in the repository root.

```bash
bash ./rodar
./app.exe
```

### Interactive preview

The interactive program opens a Raylib window with the scene preview.

```bash
bash ./rodar
./app_interativo.exe
```

Run these commands from the repository root. The scene uses relative paths for `models/` assets.

## Interactive controls

| Input | Action |
| --- | --- |
| `W` / `S` | Move camera forward / backward |
| `A` / `D` | Move camera left / right |
| `E` / `Q` | Move camera up / down |
| `Left Shift` | Increase movement speed |
| Hold right mouse button | Mouse look |
| Mouse wheel | Zoom camera window |
| Left mouse button | Pick or clear the selected object |
| `Esc` | Toggle cursor capture |
| `Tab` | Hide or restore the HUD |
| `1` / `2` / `3` | Perspective / orthographic / oblique projection |
| `Z` / `X` | Cabinet / cavalier oblique projection |
| `F1` / `F2` / `F3` | One-, two-, and three-point perspective presets |
| `L` | Cycle point, directional, and spot lights |
| `[` / `]` | Decrease / increase spot cutoff while the spot is active |
| `C` | Clear selection |
| Window close button | Exit |

## Requirements

The project has been validated on Windows using Git Bash, GCC/G++, and C++17. `rodar` rebuilds the vendored Raylib sources and links the interactive executable against the Windows system libraries `opengl32`, `gdi32`, `winmm`, `ole32`, `user32`, and `shell32`.

## Tests

Build and run each test from the repository root:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc/utils -Isrc/objetos tests/primitives_regression.cpp -o tests/primitives_regression.exe
./tests/primitives_regression.exe

g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc/utils -Isrc/objetos tests/mesh_lighting_regression.cpp -o tests/mesh_lighting_regression.exe
./tests/mesh_lighting_regression.exe

g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc/utils -Isrc/objetos tests/obj_loading_regression.cpp -o tests/obj_loading_regression.exe
./tests/obj_loading_regression.exe
```

The generated executables are ignored by Git.

## Repository structure

```text
src/main.cpp              Offline renderer
src/main_interativo.cpp   Interactive Raylib application
src/objetos/              Primitive geometry
src/utils/                Math, camera, lighting, OBJ, texture, and transform utilities
models/                   OBJ models and PPM texture assets
tests/                    Regression tests
external/raylib/          Vendored Raylib source
rodar                     Windows/Git Bash build script
```

## Project history

This repository began as an academic Computer Graphics assignment at UFC. The current version was subsequently reviewed, corrected, tested, and prepared for portfolio presentation. The Git history records one email address under the names `Ed1l3udo` and `Edileudo`; it does not provide reliable evidence to attribute every original academic contribution individually.

## Dependencies and asset credits

- Raylib 5.5 is vendored in `external/raylib/`; GLFW is included as part of its desktop backend source tree.
- The scene uses `models/patinho.obj`, `models/notebook.obj`, and `models/flat_earth03.ppm`.

The checkout does not document the original source or license of the model and texture assets, and it does not include a Raylib license text alongside the vendored source. Confirm asset permissions and dependency-license documentation before public redistribution.

## Limitations

The current renderer does not implement a BVH acceleration structure, reflections, refractions, or anti-aliasing. Rendering is therefore intended for small educational scenes rather than high-performance production workloads.
