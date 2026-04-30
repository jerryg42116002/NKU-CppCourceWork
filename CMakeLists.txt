cmake_minimum_required(VERSION 3.21)

project(TinyRayStudio
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets)
find_package(OpenGL REQUIRED)

add_executable(TinyRayStudio WIN32
    src/main.cpp
    src/gui/MainWindow.h
    src/gui/MainWindow.cpp
    src/gui/GLPreviewWidget.h
    src/gui/GLPreviewWidget.cpp
    src/gui/RealTimeRenderWidget.h
    src/gui/RealTimeRenderWidget.cpp
    src/gui/RenderWidget.h
    src/gui/RenderWidget.cpp
    src/gui/ScenePanel.h
    src/gui/ScenePanel.cpp
    src/gui/ScenePanelSelection.cpp
    src/core/Vec3.h
    src/core/MathUtils.h
    src/core/Random.h
    src/core/Ray.h
    src/core/Camera.h
    src/core/OrbitCamera.h
    src/core/OrbitCamera.cpp
    src/core/Material.h
    src/core/HitRecord.h
    src/core/Object.h
    src/core/Box.h
    src/core/Box.cpp
    src/core/Sphere.h
    src/core/Sphere.cpp
    src/core/Cylinder.h
    src/core/Cylinder.cpp
    src/core/Plane.h
    src/core/Plane.cpp
    src/core/Light.h
    src/core/Scene.h
    src/core/Scene.cpp
    src/core/SceneIO.h
    src/core/SceneIO.cpp
    src/core/RayTracer.h
    src/core/RayTracer.cpp
    src/core/RenderSettings.h
    src/core/Renderer.h
    src/core/Renderer.cpp
    src/interaction/Picking.h
    src/interaction/Picking.cpp
    src/interaction/DragMode.h
)

target_include_directories(TinyRayStudio
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(TinyRayStudio
    PRIVATE
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        OpenGL::GL
)

if(MSVC)
    target_compile_options(TinyRayStudio PRIVATE /W4)
else()
    target_compile_options(TinyRayStudio PRIVATE -Wall -Wextra -Wpedantic)
endif()
