# TinyRay Studio

TinyRay Studio 是一个小型计算机图形学桌面工具，用于交互式编辑简单 3D 场景、实时预览、CPU 高质量光线追踪渲染和图像导出。
该工具在艺术创作，图形学教学等领域有较大应用潜力

项目采用混合渲染架构：

- **实时视口**：使用 Qt `QOpenGLWidget` 和 OpenGL，用于相机控制、选择、拖拽、材质和灯光快速预览。
- **高质量渲染**：使用 CPU Ray Tracing，用于最终图片输出，支持递归反射、折射、阴影、软阴影、Bloom 和景深。

技术栈：

- C++17 
- Qt 6 Widgets
- OpenGL
- CPU Ray Tracing

## 主要功能

- Qt 6 Widgets 主窗口和右侧控制面板
- 实时 OpenGL 视口
- CPU Ray Tracing 高质量渲染
- 默认隐藏的 High Quality Render 结果预览，可用 `Show Result Preview` 动态展开
- Orbit Camera 鼠标交互：
  - 左键拖动空白区域旋转视角
  - 右键拖动平移视角
  - 鼠标滚轮缩放
- Turntable 自动环绕相机：
  - Start / Stop Turntable
  - 速度调节
  - Clockwise / Counterclockwise 方向
  - Scene Center / Selected Object / Custom Target
  - 用户手动交互时自动暂停
- Depth of Field 景深：
  - `Aperture`
  - `Focus Distance`
  - `Focus Selected Object` 可快速设置焦点距离
  - 景深主要体现在 `High Quality Render` 的 CPU 渲染结果中
- Transform Gizmo：
  - 选中物体或光源后显示红 X、绿 Y、蓝 Z 坐标轴
  - 拖动坐标轴可沿 X/Y/Z 移动物体或光源
- 对象和光源选择、拖拽
- Overlay Label 选中信息显示
- Reset View / Focus Selected Object
- 支持几何体：
  - Sphere
  - Plane
  - Box
  - Cylinder
- 支持光源：
  - Point Light
  - Area Light
- 支持材质：
  - Diffuse
  - Metal
  - Glass / Dielectric
  - Emissive
- 支持纹理：
  - Checkerboard
  - Image Texture
- 支持环境背景：
  - Gradient
  - Solid Color
  - Image / HDR Image 路径
- 后处理：
  - Bloom
  - Exposure
- 粒子雨效：
  - 默认关闭
  - 可手动开启 Rain
  - 可调 Rain Rate、Drop Speed、Drop Lifetime、Drop Size、Gravity、Spawn Area
  - Splash 水花效果
- CPU Ray Tracing：
  - Ray generation
  - Sphere / Plane / Box / Cylinder intersection
  - Lambert diffuse
  - Shadow rays
  - Point Light hard shadow
  - Area Light soft shadow
  - Metal recursive reflection
  - Glass refraction and total internal reflection
  - Emissive hit lighting
  - Depth of Field
  - Anti-aliasing
  - Gamma correction
  - Multi-threaded rendering
  - Progressive preview
  - Render cancellation
- PNG 图像导出
- JSON 场景保存和加载
- `--self-test` 非 GUI 自检入口

## 环境要求

- Windows
- Visual Studio 2022 或 Visual Studio Build Tools
- CMake 3.21+
- Qt 6 MSVC 2022 64-bit Kit
- 支持 C++17 的编译器

项目使用 Qt Widgets、QOpenGLWidget、OpenGL 和标准 C++。不使用 CUDA、OptiX、Vulkan、DirectX、Qt WebEngine 或游戏引擎。

## 构建方法

进入项目根目录：

```powershell
cd D:\NKUcpp大作业
```

配置 CMake：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.11.0\msvc2022_64"
```

构建 Release：

```powershell
cmake --build build --config Release
```

如果你的 Qt 安装路径不同，请把 `CMAKE_PREFIX_PATH` 改为本机 Qt Kit 目录，例如该目录下应能找到：

```text
lib\cmake\Qt6\Qt6Config.cmake
```

## 部署 Qt DLL

如果双击运行时提示缺少 Qt DLL，可执行：

```powershell
C:\Qt\6.11.0\msvc2022_64\bin\windeployqt.exe D:\NKUcpp大作业\build\Release\TinyRayStudio.exe
```

如果看到 `dxcompiler.dll` 或 `VCINSTALLDIR` 相关 warning，通常可以先忽略。TinyRay Studio 使用 Qt Widgets 和 OpenGL，不依赖 DirectX 渲染管线。

## 基本使用

1. 启动 `TinyRayStudio.exe`。
2. 在左侧实时视口观察和编辑场景。
3. 使用鼠标控制相机：
   - 左键拖动空白区域旋转
   - 右键拖动平移
   - 滚轮缩放
4. 点击物体或光源进行选择。
5. 选中后可拖动物体、光源，或拖动红绿蓝 Transform Gizmo 坐标轴进行精确方向移动。
6. 右侧 `Camera` 面板可控制 Turntable、景深、Reset View 和 Focus Selected Object。
7. 点击 `High Quality Render` 启动 CPU 光线追踪。
8. 默认不显示结果预览；点击 `Show Result Preview` 可展开右侧高质量渲染预览。
9. 点击 `Save Image` 导出 PNG。
10. 使用 `Save Scene` / `Load Scene` 保存和加载 JSON 场景。

更完整的操作说明见 [USER_GUIDE.md](USER_GUIDE.md)。

## 景深 Depth of Field

景深参数位于右侧 `Camera` 面板：

- `Aperture`：孔径。`0` 表示关闭景深，数值越大虚化越明显。
- `Focus Distance`：焦点距离。焦点附近清晰，焦点前后会虚化。
- `Focus Selected Object`：选中物体后点击，可把焦点距离自动设置到该物体附近。

注意：景深效果主要出现在 `High Quality Render` 的 CPU 渲染结果里。实时 OpenGL 视口用于交互预览，不会直接模糊背景。

## Transform Gizmo

选中 Sphere、Box、Cylinder、Point Light 或 Area Light 后，实时视口会显示三根坐标轴：

- 红色：X 轴
- 绿色：Y 轴
- 蓝色：Z 轴

按住某根轴拖动即可沿对应方向移动选中对象。Gizmo 拖动会复用现有移动逻辑，因此 Scene Panel、Overlay Label、Turntable 自动暂停、CPU 渲染快照都能保持一致。

## 粒子雨效

雨效默认关闭。需要时在 `Particles / Rain` 面板勾选 `Enable Rain`。

可调参数包括：

- `Rain Rate`
- `Drop Speed`
- `Drop Lifetime`
- `Gravity`
- `Spawn Area Size`
- `Enable Splash`
- `Splash Intensity`
- `Drop Size`

关闭 Rain 时会清空当前粒子，避免雨滴残留遮挡画面。

## 项目结构

```text
src/
  core/          数学、射线、相机、材质、纹理、环境、光源、几何体、场景、序列化、CPU 渲染
  gui/           Qt Widgets 界面、实时视口、渲染结果预览、场景面板
  interaction/   picking、Overlay 投影、鼠标交互辅助
  particle/      CPU 粒子系统、RainEmitter、SplashEmitter
  preview/       OpenGL Bloom、Skybox / Environment 辅助
```

## 图形学技术点

- Ray Tracing 基础框架
- 射线与 Sphere / Plane / Box / Cylinder 求交
- 法线和 front-face 判断
- Point Light 硬阴影
- Area Light 多采样软阴影
- shadow ray 和 epsilon 偏移
- Lambert 漫反射
- Metal 递归反射
- Glass 折射、全反射、Schlick 近似
- Emissive 自发光材质
- Bloom 泛光后处理
- Exposure 色调控制
- 随机子像素采样
- gamma correction
- 多线程 CPU 渲染
- 渐进式预览
- OpenGL / GLSL 实时渲染视口
- Orbit Camera
- Turntable 相机动画
- 鼠标 picking
- 对象和光源拖拽
- Overlay Label 屏幕投影
- Checkerboard / Image Texture
- Environment Background / Skybox 预览

## 后续计划

- 实时视口近似软阴影
- Area Light 旋转交互手柄
- 更低噪声的分层采样 / 蓝噪声采样
- Emissive 参与真实间接照明
- 更高级的色调映射和曝光控制
- 更完整的材质与灯光预设
