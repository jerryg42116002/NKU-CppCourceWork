# AGENTS.md

## Project Name

TinyRay Studio

## Project Type

C++ graphical desktop application.

This is not a game. It is a computer graphics tool for visualizing CPU-based ray tracing.

## Main Goal

Build a Qt Widgets desktop application that allows users to create and preview a simple 3D scene, then render it using a CPU ray tracer.

The final program should demonstrate core computer graphics concepts:

- Ray generation from camera
- Ray-object intersection
- Diffuse material
- Metal reflection
- Glass refraction
- Hard shadow
- Multiple point lights
- Anti-aliasing
- Recursive ray tracing
- Gamma correction
- Multi-threaded tile rendering
- Progressive rendering preview
- Image export

## Technology Constraints

Use:

- C++17 or newer
- Qt 6 Widgets
- CMake
- QImage for displaying rendered images
- Standard C++ threading tools

Avoid:

- CUDA
- Vulkan
- DirectX
- Game engines
- Python
- Web frontend
- Heavy external dependencies
- Platform-specific hacks

OpenGL is not required. The ray tracing renderer should run on CPU.

## Code Quality Rules

1. Keep the code modular.
2. Do not put all logic in main.cpp.
3. Separate GUI code from rendering code.
4. Separate math, scene, object, material, camera, and renderer modules.
5. Prefer clear class names and readable code over clever tricks.
6. Add comments for important ray tracing formulas.
7. Avoid global mutable state.
8. Use RAII and smart pointers where appropriate.
9. Make sure the project builds with CMake.
10. Keep the program runnable even if optional features are incomplete.

## GUI Requirements

The main window should include:

- A central render preview area
- A left or right scene control panel
- A render settings panel
- Buttons:
  - Render
  - Stop
  - Clear
  - Save Image
  - Load Preset
  - Save Scene
  - Load Scene
- Controls for:
  - Image width
  - Image height
  - Samples per pixel
  - Max ray depth
  - Number of threads
  - Background color
  - Camera position
  - Camera look-at point
  - Field of view

## Scene Editing Requirements

Support at least:

- Add sphere
- Add plane
- Delete selected object
- Edit object transform
- Edit object material
- Edit point light
- Load preset scene

Supported materials:

- Diffuse / Lambertian
- Metal
- Glass / Dielectric
- Emissive, optional

## Rendering Requirements

The renderer should support:

- Ray-sphere intersection
- Ray-plane intersection
- Point light direct illumination
- Shadow rays
- Recursive reflection
- Recursive refraction
- Anti-aliasing via random subpixel sampling
- Gamma correction
- Multi-threaded rendering by tiles or rows
- Render cancellation
- Progress reporting
- Progressive image update if feasible

## Error Handling

The application should not crash when:

- There are no objects
- There are no lights
- The user stops rendering
- A scene file is invalid
- A render resolution is too large

Show user-friendly error messages in the GUI.

## Performance Constraints

The renderer should be usable on normal laptops.

Default settings:

- Resolution: 800 x 450
- Samples per pixel: 16
- Max depth: 5
- Threads: hardware_concurrency

High-quality settings can be optional.

## Visual Style

The GUI should look like a small professional graphics tool.

Use clear grouping:

- Scene
- Camera
- Material
- Render Settings
- Output

The default scene should look visually impressive:

- A reflective metal sphere
- A glass sphere
- A diffuse colored sphere
- A ground plane
- Two colored lights
- Dark gradient-like background if possible

## Development Strategy

Implement the project in stages:

1. Basic Qt window and image preview
2. Minimal CPU ray tracer
3. Sphere and plane intersection
4. Diffuse shading and shadows
5. Reflection and refraction
6. Anti-aliasing and gamma correction
7. Multi-threading and progress update
8. GUI scene editing
9. Save rendered image
10. Scene preset save/load
11. Polish UI and README

Do not attempt all advanced features at once.
Make every stage compile and run before adding the next feature.

## Expected Final Result

The final application should allow the user to open the program, load a preset scene, click Render, watch the image progressively appear, adjust camera/material/render settings, and save the result as PNG.

The program should be suitable for a university C++ graphical programming assignment and should demonstrate creativity, technical depth, and computer graphics knowledge.