# TinyRay Studio

TinyRay Studio 是一个基于 **C++17 + Qt 6 Widgets + CMake** 的 CPU 光线追踪渲染器 GUI 项目。

项目定位是计算机图形学工具，不是游戏。用户可以在 Qt 桌面窗口中设置渲染参数，点击 Render 后由 CPU 光线追踪器生成图像，并在中央预览区域显示结果。

## 当前功能

- Qt Widgets 主窗口
- 中央 `RenderWidget` 渲染预览区
- 右侧 Render Settings 参数面板
- 底部状态栏和渲染进度条
- Render、Stop、Save Image、Clear 按钮
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
3. 点击 `Render` 开始渲染默认场景。
4. 渲染完成后可点击 `Save Image` 保存 PNG。
5. 点击 `Clear` 可以清空当前预览。
6. `Stop` 当前保留取消接口，后续多线程或渐进式渲染阶段会继续完善。

## 后续计划

- 递归路径追踪
- 多线程 tile 渲染
- 渐进式预览
- GUI 场景编辑
- 场景保存和加载
