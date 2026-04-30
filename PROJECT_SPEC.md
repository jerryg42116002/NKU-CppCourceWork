# PROJECT_SPEC.md

本文档保存 TinyRay Studio 的完整项目说明。只有在进行高推理写代码、架构设计或较大功能开发时才需要读取。

## 项目名称

TinyRay Studio

## 项目类型

C++ 图形化桌面应用。它不是游戏，而是用于可视化 CPU 光线追踪的计算机图形学工具。

## 主要目标

构建一个 Qt Widgets 桌面程序，让用户可以创建、预览并渲染简单 3D 场景。程序使用 CPU ray tracing 生成图像，并通过 GUI 显示和导出结果。

最终项目应展示以下图形学概念：

- 相机射线生成
- 射线与物体求交
- Diffuse / Lambertian 材质
- Metal 反射
- Glass / Dielectric 折射
- 硬阴影
- 点光源直接光照
- 多采样抗锯齿
- 递归光线追踪
- Gamma correction
- 多线程 CPU 渲染
- Progressive rendering preview
- 图片导出

## 技术约束

使用：

- C++17 或更新版本
- Qt 6 Widgets
- CMake
- QImage 显示和保存渲染结果
- 标准 C++ threading 工具

避免：

- CUDA
- Vulkan
- DirectX
- 游戏引擎
- Python 运行时依赖
- Web frontend
- 重型外部依赖
- 平台专用 hack

OpenGL 不是必需项。光线追踪渲染器应运行在 CPU 上。

## 代码质量规则

1. 保持代码模块化。
2. 不要把所有逻辑写进 `main.cpp`。
3. GUI 代码与渲染代码分离。
4. 数学、场景、物体、材质、相机、渲染器模块分离。
5. 优先使用清晰类名和可读代码。
6. 重要光线追踪公式需要写简洁注释。
7. 避免全局可变状态。
8. 使用 RAII 和智能指针。
9. 项目必须通过 CMake 构建。
10. 可选功能未完成时，程序仍应可运行。
11. CPU ray tracing core 尽量独立于 Qt。
12. Qt 主要用于 GUI、图像显示、事件和文件对话框。

## GUI 目标

主窗口应包含：

- 中央 render preview 区域
- 左侧或右侧 scene/render control panel
- render settings panel
- 状态栏显示进度和消息

按钮：

- Render
- Stop
- Clear
- Save Image
- Load Preset
- Save Scene
- Load Scene

控件：

- Image width
- Image height
- Samples per pixel
- Max ray depth
- Number of threads
- Background color
- Camera position
- Camera look-at point
- Field of view

GUI 在渲染过程中必须保持响应。不得从 worker thread 直接更新 QWidget，必须使用 Qt signal/slot。

## 场景编辑目标

后续至少支持：

- Add sphere
- Add plane
- Delete selected object
- Edit object transform
- Edit object material
- Edit point light
- Load preset scene

支持材质：

- Diffuse / Lambertian
- Metal
- Glass / Dielectric
- Emissive 可选

## 渲染目标

渲染器应支持：

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
- Progressive image update

渲染核心应尽量可在不启动 GUI 的情况下使用。

## 默认设置

- Resolution: 800 x 450
- Samples per pixel: 16
- Max depth: 5
- Threads: `std::thread::hardware_concurrency()`

## 默认场景视觉目标

默认场景应具有辨识度，至少包含：

- 地面平面
- 多个彩色 diffuse 球体
- 一个 metal 反射球
- 一个 glass 折射球
- 点光源
- 背景渐变
- 明暗变化和投影阴影

## 错误处理目标

程序不应在以下情况崩溃：

- 没有物体
- 没有光源
- 用户停止渲染
- 分辨率较大
- 场景文件无效

GUI 应显示友好错误信息。

## 开发路线

1. Basic Qt window and image preview
2. Minimal CPU ray tracer
3. Sphere and plane intersection
4. Diffuse shading and shadows
5. Reflection and refraction
6. Anti-aliasing and gamma correction
7. Multi-threading and progress update
8. Progressive rendering preview
9. GUI scene editing
10. Save rendered image
11. Scene preset save/load
12. Polish UI and README

每个阶段都应保持项目可构建、可运行，再进入下一阶段。

## 当前已实现概览

- Qt Widgets 主窗口
- `RenderWidget` 使用 QImage 显示图像
- 右侧 Render Settings 面板
- Render / Stop / Save Image / Clear
- 状态栏进度和消息
- `Vec3`、`Ray`、`Camera`
- `Material`：Diffuse、Metal、Glass
- `Sphere`、`Plane`、`Light`、`Scene`
- `Renderer`、`RayTracer`、`RenderSettings`
- 直接光照和阴影
- Metal 递归反射
- Glass 折射、全反射和 Schlick 近似
- SPP 随机子像素采样
- Gamma 2.2 correction
- 多线程行渲染
- Progressive preview
- Scene Panel 对象列表和属性编辑
- Sphere / Plane / Point Light 添加与删除
- Reflection / Glass / Colored Lights 内置预设
- JSON scene save/load
- File / Render / Help 菜单栏
- About 窗口
- PNG 导出路径选择
- OpenGL Preview tab
- Render Result tab
- OrbitCamera 实时视角旋转、缩放和平移
