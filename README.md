# TinyRay Studio

TinyRay Studio 是一个基于 **C++17 + Qt 6 Widgets + CMake** 的计算机图形学桌面工具。项目采用“单一实时渲染视口 + 可选 CPU Ray Tracing 高质量渲染”的混合架构：中央视口用于即时观察、选择、拖动物体和光源；CPU 光线追踪用于生成最终高质量图片。

它不是游戏，而是一个用于展示和实验基础图形学算法的小型图形工具。

## 当前功能

- Qt 6 Widgets 主窗口和右侧参数面板
- 中央实时渲染视口 `RealTimeRenderWidget`
- OpenGL / GLSL 实时渲染当前场景
- Orbit Camera 视角控制：
  - 左键拖动空白区域旋转视角
  - 右键拖动平移视角
  - 鼠标滚轮缩放
- 场景对象点击选中
- Sphere / Box / Cylinder / Point Light / Area Light 拖拽移动
- 拖拽轴模式切换
- Scene Panel 对象列表和 Inspector 属性编辑
- 支持对象：
  - Sphere
  - Plane
  - Box
  - Cylinder
- 支持光源：
  - Point Light 点光源
  - Area Light 矩形面光源
- 支持材质：
  - Diffuse 漫反射
  - Metal 金属反射
  - Glass / Dielectric 玻璃折射
  - Emissive 自发光材质
- 后处理：
  - Bloom 泛光
  - Exposure 曝光控制
- CPU Ray Tracing 高质量渲染：
  - 相机射线生成
  - Sphere / Plane / Box / Cylinder 求交
  - Point Light 硬阴影
  - Area Light 多采样软阴影
  - Lambert 漫反射
  - Metal 递归反射
  - Glass 折射、全反射和 Schlick 近似
  - Emissive 自发光命中
  - 多重采样抗锯齿
  - gamma correction
  - 多线程渲染
  - 渐进式图像更新
  - Stop 取消渲染
- PNG 图片导出
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

如果你的 Qt 安装路径不同，请把 `CMAKE_PREFIX_PATH` 改成自己的 Qt Kit 路径。

## 部署 Qt DLL

如果双击运行时提示缺少 Qt DLL，可以执行：

```powershell
C:\Qt\6.11.0\msvc2022_64\bin\windeployqt.exe D:\NKUcpp大作业\build\Release\TinyRayStudio.exe
```

如果看到 `dxcompiler.dll` 或 `VCINSTALLDIR` 相关 warning，一般可以先忽略；TinyRay Studio 不依赖 DirectX 渲染管线。

## 使用方法

1. 启动 `TinyRayStudio.exe`。
2. 在中央实时视口中观察当前场景。
3. 使用鼠标控制视角：
   - 左键拖动空白区域旋转
   - 右键拖动平移
   - 滚轮缩放
4. 点击对象或光源进行选中。
5. 拖动物体、点光源或面光源，实时查看场景变化。
6. 在右侧 Inspector 中修改对象位置、尺寸、材质、光源和后处理参数。
7. 点击 `High Quality Render` 使用 CPU Ray Tracing 生成最终图片。
8. 点击 `Save Image` 导出 PNG。
9. 使用 `Save Scene` / `Load Scene` 保存和加载 JSON 场景。

更完整的操作说明见 [USER_GUIDE.md](USER_GUIDE.md)。

## Emissive 发光材质

Emissive 是材质层面的自发光。它不依赖 Point Light 或 Area Light，也能在实时视口和 CPU 渲染结果中可见。

Inspector 中可以设置：

- `Material = Emissive`
- `Emission Color`：发光颜色
- `Emission Strength`：发光强度

当前 Emissive 物体会自己发亮，并能触发实时 Bloom；它仍然和 Area Light 是两个不同概念。

## Area Light 与 Soft Shadows

Area Light 是显式光源对象，当前实现为矩形面光源。它包含：

- `position`：中心位置
- `normal`：朝向
- `width`
- `height`
- `color`
- `intensity`

CPU Ray Tracing 中，Area Light 会对矩形表面进行多点采样，从 shading point 向采样点发射 shadow ray，统计可见样本贡献并平均，从而产生软阴影。Point Light 仍保留原有硬阴影路径。

实时视口中，Area Light 会显示为可见的发光矩形，支持选中和拖拽；实时视口目前主要用于快速交互，不强制计算真实软阴影。软阴影效果主要在 `High Quality Render` 的 CPU 渲染结果中体现。

## Bloom 泛光

实时视口支持 Bloom 后处理：

- 提取亮部区域
- 对亮部做水平/垂直模糊
- 与原场景合成
- 使用 Exposure 控制最终显示亮度

右侧 `Post Processing` 面板可调节：

- Bloom 开关
- Exposure
- Threshold
- Strength
- Blur Passes

如果已经生成 CPU 高质量渲染结果，`Save Image` 优先保存 CPU 图像；否则保存当前实时视口截图，截图包含当前 Bloom 效果。

## 项目结构

```text
src/
  core/          数学、射线、相机、材质、光源、几何体、场景、序列化、CPU 渲染
  gui/           Qt Widgets 界面、实时渲染视口、场景面板
  interaction/   picking、鼠标交互辅助逻辑
  preview/       OpenGL 后处理与 Bloom
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
- 鼠标 picking
- 对象和光源拖拽

## 后续计划

- 实时视口近似软阴影
- Area Light 旋转交互手柄
- 更低噪声的分层采样 / 蓝噪声采样
- Emissive 参与真实间接照明
- 更高级的色调映射和曝光控制
- 更完整的材质与灯光预设
