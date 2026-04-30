cmake_minimum_required(VERSION 3.21)

project(TinyRayStudio
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(TinyRayStudio WIN32
    src/main.cpp
    src/gui/MainWindow.h
    src/gui/MainWindow.cpp
    src/gui/RenderWidget.h
    src/gui/RenderWidget.cpp
    src/core/Vec3.h
    src/core/MathUtils.h
    src/core/Random.h
    src/core/Ray.h
    src/core/Camera.h
    src/core/Material.h
    src/core/HitRecord.h
    src/core/Object.h
    src/core/Sphere.h
    src/core/Sphere.cpp
    src/core/Plane.h
    src/core/Plane.cpp
    src/core/Light.h
    src/core/Scene.h
    src/core/Scene.cpp
    src/core/RayTracer.h
    src/core/RayTracer.cpp
    src/core/RenderSettings.h
    src/core/Renderer.h
    src/core/Renderer.cpp
)

target_include_directories(TinyRayStudio
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(TinyRayStudio
    PRIVATE
        Qt6::Widgets
)

if(MSVC)
    target_compile_options(TinyRayStudio PRIVATE /W4)
else()
    target_compile_options(TinyRayStudio PRIVATE -Wall -Wextra -Wpedantic)
endif()
