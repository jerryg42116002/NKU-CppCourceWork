# AGENTS.md

## Project Name

TinyRay Studio

## Project Type

C++ graphical desktop application.

This is not a game. It is a computer graphics tool for interactive scene editing, real-time OpenGL preview, and CPU-based ray tracing final rendering.

---

## Main Goal

Build a C++17 / Qt 6 Widgets desktop application for visualizing and editing simple 3D scenes.

The program should support:

1. Real-time OpenGL preview for interaction.
2. CPU ray tracing for high-quality final rendering.
3. Scene editing through GUI.
4. Mouse-based camera control.
5. Object selection and dragging.
6. Image export.

The final application should feel like a small graphics tool, not a static demo.

---

## Rendering Architecture

This project uses a hybrid rendering architecture:

### 1. OpenGL Real-Time Preview Renderer

Used for:

- Real-time scene preview
- Mouse camera rotation
- Mouse wheel zoom
- View panning
- Object selection
- Object dragging
- Quick material and lighting preview

The OpenGL preview should be implemented with:

- Qt 6 Widgets
- QOpenGLWidget
- QOpenGLFunctions
- OpenGL
- GLSL if needed

### 2. CPU Ray Tracing Final Renderer

Used for high-quality final rendering.

The CPU ray tracer should implement core computer graphics algorithms:

- Ray generation from camera
- Ray-object intersection
- Sphere intersection
- Plane intersection
- Point light illumination
- Lambert diffuse shading
- Shadow rays
- Metal reflection
- Glass refraction
- Recursive ray tracing
- Anti-aliasing
- Gamma correction
- Multi-threaded rendering
- Progressive image update if feasible
- Render cancellation
- PNG image export

---

## Technology Constraints

### Use

- C++17 or newer
- Qt 6 Widgets
- QOpenGLWidget
- OpenGL
- CMake
- QImage
- Standard C++ threading tools
- Qt signal / slot for GUI-thread communication

### Avoid

- CUDA
- OptiX
- Vulkan
- DirectX
- Game engines
- Python
- Web frontend
- Qt WebEngine
- Heavy external dependencies
- Platform-specific hacks

### Notes

The project may use the GPU through OpenGL for real-time preview.

The main ray tracing algorithm should still be implemented in C++ on the CPU.

Do not replace the CPU ray tracer with CUDA, OptiX, Vulkan Ray Tracing, or DirectX Raytracing unless explicitly requested.

---

## Core Workflow Rule

This repository uses a strict two-agent workflow:

1. Implementation Agent
2. Validation Agent

The two roles must not be mixed.

---

# Implementation Agent Rules

Use this role when implementing features, modifying code, refactoring, or fixing bugs.

## Allowed

- Read source files.
- Modify source files.
- Add new source files.
- Modify CMake files.
- Modify documentation.
- Refactor code.
- Add interfaces.
- Add self-test logic.
- Update `VALIDATION_PLAN.md`.

## Forbidden

- Do not run shell commands.
- Do not run CMake.
- Do not build the project.
- Do not run tests.
- Do not launch the application.
- Do not claim the code has been compiled.
- Do not claim tests passed.
- Do not say the change is verified unless validation logs are provided.

## Required After Every Implementation Task

Create or update:

```text
VALIDATION_PLAN.md