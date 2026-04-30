# TinyRay Studio

TinyRay Studio 是一个基于 **C++17 + Qt 6 Widgets + CMake** 的 CPU 光线追踪渲染器 GUI 项目。

项目定位是计算机图形学工具，不是游戏。用户可以在 Qt 桌面窗口中设置渲染参数，点击 Render 后由 CPU 光线追踪器生成图像，并在中央预览区域显示结果。

## 当前功能

- Qt Widgets 主窗口
- 中央 `RenderWidget` 渲染预览区
- 右侧 Render Settings 参数面板
- 底部状态栏和渲染进度条
- Render、Stop、Save Image、Clear 按钮
- File / Render / Help 菜单栏
- Save Scene / Load Scene JSON 场景保存加载
- 右侧 Scene Panel 对象列表和属性编辑器
- 内置预设场景：Reflection Demo、Glass Demo、Colored Lights Demo
- 使用 `QImage` 显示和保存 PNG
- 基础数学模块：`Vec3`、`Ray`、`Camera`、随机数和数学工具
- 基础场景模块：`Object`、`HitRecord`、`Sphere`、`Plane`、`Light`、`Scene`
- 默认场景：地面、三个球体、一个点光源
- CPU Ray Tracing 基础渲染：
  - 相机发射射线
  - 球体和平面求交
  - Diffuse 材质 albedo
  - 环境光
  - Lambert 漫反射
  - 点光源直接光照
  - 阴影射线
  - epsilon 偏移减少 shadow acne
  - Metal 金属反射材质
  - roughness 控制模糊反射
  - Glass / Dielectric 折射材质
  - Snell's Law 折射和全反射
  - Schlick 近似反射概率
  - samples per pixel 多采样平均
  - gamma 2.2 校正
  - max depth 控制递归深度
  - 多线程 CPU 渲染
  - Progressive Rendering 渐进式预览

## 环境要求

- CMake 3.21 或更新版本
- Qt 6 Widgets
- 支持 C++17 的编译器

项目只使用 Qt Widgets 和标准 C++。不使用 OpenGL、CUDA、Vulkan、DirectX、Web 前端或游戏引擎。

## 构建方法

Windows 下如果使用 Qt 官方安装器，可以按类似下面的方式指定 Qt 6 路径：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.6.0/msvc2019_64"
cmake --build build
```

请根据本机安装的 Qt 6 Kit 修改 `CMAKE_PREFIX_PATH`。

如果 Qt 6 已经加入系统环境变量，也可以直接执行：

```powershell
cmake -S . -B build
cmake --build build
```

## 项目结构

```text
src/
  core/   数学、射线、相机、材质、几何体、场景、光照和渲染器
  gui/    Qt Widgets 界面代码
```

核心渲染逻辑与 GUI 分离，便于后续继续扩展材质、递归反射、折射、抗锯齿、多线程和渐进式渲染。

## 使用方法

1. 启动程序。
2. 在右侧面板设置分辨率、samples per pixel、max depth 和线程数。
3. 在 Presets 中选择内置预设，或在 Scene 面板中添加、删除和编辑 Sphere、Plane、Point Light。
4. 点击 `Render` 开始渲染当前场景。
5. 渲染过程中可以看到渐进式预览，也可以点击 `Stop` 保留当前半成品图像。
6. 点击 `Save Image` 可以选择路径导出 PNG。
7. 通过菜单或按钮可以 `Save Scene` / `Load Scene` 保存和加载 JSON 场景。
8. 点击 `Clear` 可以清空当前预览。

## 图形学技术点

- CPU ray tracing
- Ray generation from perspective camera
- Ray-sphere intersection
- Ray-plane intersection
- Front-face normal handling
- Lambert diffuse lighting
- Point light shadow ray
- Shadow acne epsilon offset
- Recursive metal reflection
- Glass refraction using Snell's Law
- Total internal reflection
- Schlick reflectance approximation
- Random subpixel sampling
- Gamma correction
- Multi-threaded row rendering
- Progressive preview

## 后续计划

- 递归路径追踪
- 更完整的材质编辑
- 相机 GUI 编辑
- 场景文件版本兼容
- 更精致的 UI 主题
