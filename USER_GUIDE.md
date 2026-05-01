# TinyRay Studio 使用文档

本文档面向第一次使用 TinyRay Studio 的用户，按“环境准备、实时交互、编辑场景、高质量渲染、保存结果”的顺序说明。

## 1. 软件简介

TinyRay Studio 是一个基于 **C++17 + Qt 6 Widgets + CMake** 的图形学桌面工具。

程序中央是实时渲染视口，用户可以旋转视角、缩放、选择对象、拖动物体和光源。需要最终高质量图片时，再点击 `High Quality Render` 使用 CPU Ray Tracing 渲染当前场景。

当前支持：

- 实时 OpenGL / GLSL 渲染视口
- Sphere / Plane / Box / Cylinder
- Point Light 点光源
- Area Light 矩形面光源
- Diffuse / Metal / Glass / Emissive 材质
- Bloom 泛光
- Exposure 曝光控制
- Point Light 硬阴影
- Area Light 软阴影
- 反射、折射、Schlick 近似
- 多采样抗锯齿
- 多线程 CPU 渲染
- PNG 导出
- JSON 场景保存和加载

## 2. 运行前准备

如果你已经有可以运行的 `TinyRayStudio.exe`，并且同目录下已经部署 Qt DLL，可以直接双击运行。

如果需要自己编译，请安装：

- Visual Studio 2022 或 Visual Studio Build Tools
- CMake
- Qt 6 MSVC 2022 64-bit Kit

推荐 Qt 路径示例：

```text
C:\Qt\6.11.0\msvc2022_64
```

## 3. 编译项目

在 PowerShell 中进入项目目录：

```powershell
cd D:\NKUcpp大作业
```

配置 CMake：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.11.0\msvc2022_64"
```

编译 Release：

```powershell
cmake --build build --config Release
```

编译完成后程序一般位于：

```text
D:\NKUcpp大作业\build\Release\TinyRayStudio.exe
```

## 4. 部署 Qt DLL

如果双击程序提示缺少 DLL，运行：

```powershell
C:\Qt\6.11.0\msvc2022_64\bin\windeployqt.exe D:\NKUcpp大作业\build\Release\TinyRayStudio.exe
```

完成后再次双击 `TinyRayStudio.exe`。

如果出现 `dxcompiler.dll` 或 `VCINSTALLDIR` 相关 warning，一般可以先忽略。TinyRay Studio 使用 Qt Widgets 和 OpenGL，不依赖 DirectX 渲染管线。

## 5. 主界面说明

程序打开后主要分为三部分：

1. 中央实时视口  
   显示当前场景。拖动视角、拖动物体、修改材质、修改光源或修改 Bloom 参数后，画面会立即更新。

2. 右侧控制面板  
   包含预设场景、对象列表、Inspector 属性编辑器、阴影参数、Bloom 参数和渲染参数。

3. 底部状态栏  
   显示当前状态，例如 `Real-time rendering`、`Object dragging`、`High quality rendering`、`Render done`。

窗口可以自由调整大小。右侧面板内容较多时，可以滚动查看。

## 6. 实时视口操作

鼠标操作：

- 左键拖动空白区域：旋转视角
- 右键拖动：平移视角
- 滚轮：缩放视角
- 左键点击物体：选中物体
- 左键拖动物体：移动物体
- 左键点击光源：选中光源
- 左键拖动光源：移动光源

当前支持拖拽：

- Sphere
- Box
- Cylinder
- Point Light
- Area Light

Plane 通常作为地面，不建议拖动。

## 7. 拖拽方向

如果界面中提供拖拽模式或移动轴选项，可以选择：

- XZ 平面移动：适合在地面上移动物体
- X 轴移动：只改变 X 坐标
- Y 轴移动：只改变高度
- Z 轴移动：只改变 Z 坐标

摆放物体时推荐使用 XZ 平面移动；调整光源高度时推荐使用 Y 轴移动。

## 8. 预设场景

右侧 `Presets` 下拉框提供内置场景：

- `Colored Lights Demo`  
  展示彩色灯光、金属、玻璃、Emissive 发光物体、Area Light 和 Bloom。

- `Reflection Demo`  
  展示金属反射、彩色球体、地面和基础光照。

- `Glass Demo`  
  展示玻璃折射、反射、全反射和多光源效果。

选择预设后，当前场景会被替换，对象列表会自动刷新。

## 9. 场景对象列表

右侧 `Scene` 区域显示当前场景中的对象和光源。

常见条目：

```text
Sphere 1 (Diffuse)
Sphere 2 (Metal)
Sphere 3 (Glass)
Box 1 (Emissive)
Cylinder 1 (Metal)
Plane 1 (Diffuse)
Point Light 1
Area Light 2
```

点击条目后，下方 `Inspector` 会显示该对象或光源的可编辑属性。

## 10. 添加和删除对象

可以通过右侧按钮添加：

- `Sphere`
- `Box`
- `Cyl`
- `Plane`
- `Point`
- `Area`

其中：

- `Point` 添加点光源
- `Area` 添加矩形面光源

选中对象或光源后，点击 `Delete` 可以删除。

## 11. 编辑几何体

Sphere 可编辑：

- `Center X / Y / Z`
- `Radius`
- `Material`
- `Albedo`
- `Roughness`
- `Refractive Index`
- `Emission Color`
- `Emission Strength`

Box 可编辑：

- `Center X / Y / Z`
- `Size X / Y / Z`
- `Material`
- `Albedo`
- `Roughness`
- `Refractive Index`
- `Emission Color`
- `Emission Strength`

Cylinder 可编辑：

- `Center X / Y / Z`
- `Radius`
- `Height`
- `Material`
- `Albedo`
- `Roughness`
- `Refractive Index`
- `Emission Color`
- `Emission Strength`

Plane 可编辑：

- `Point X / Y / Z`
- `Normal X / Y / Z`
- `Material`
- `Albedo`
- `Roughness`
- `Refractive Index`
- `Emission Color`
- `Emission Strength`

修改参数后，中央实时视口会立即更新。点击 `High Quality Render` 后，CPU 渲染会使用最新场景。

## 12. 材质说明

### Diffuse

漫反射材质，适合普通彩色物体和地面。

### Metal

金属反射材质。`Roughness` 越低，反射越清晰；越高，反射越模糊。

### Glass

玻璃折射材质。`Refractive Index` 常用值为 `1.5`。

### Emissive

自发光材质。设置方法：

1. 选中 Sphere、Box、Cylinder 或 Plane。
2. 将 `Material` 改成 `Emissive`。
3. 设置 `Emission Color`。
4. 调整 `Emission Strength`。

建议：

```text
Emission Strength = 3.0 到 8.0
```

Emissive 是材质效果，Area Light 是显式光源，两者不是同一个概念。

## 13. 编辑 Point Light

选中 Point Light 后，可以编辑：

- `Position X / Y / Z`
- `Color`
- `Intensity`

Point Light 在 CPU 渲染中使用单条 shadow ray，因此阴影边缘较硬。

## 14. 编辑 Area Light

选中 Area Light 后，可以编辑：

- `Position X / Y / Z`：矩形面光源中心位置
- `Normal X / Y / Z`：矩形朝向
- `Width`：矩形宽度
- `Height`：矩形高度
- `Color`：光源颜色
- `Intensity`：光源强度

Area Light 会在实时视口中显示为发光矩形。选中后会高亮显示，并且可以像点光源一样拖拽移动。

CPU 高质量渲染中，Area Light 会产生软阴影。

## 15. 软阴影设置

右侧 `Shadows` 分组包含：

- `Enabled`：是否开启 Area Light 软阴影
- `Area Samples`：面光源采样数

推荐值：

```text
Area Samples = 8 或 16
```

采样数越高，软阴影越平滑，但 CPU 渲染越慢。关闭软阴影后，Area Light 会退化为较低采样的近似效果。

## 16. Bloom 和曝光

右侧 `Post Processing` 分组包含：

- `Enabled`：Bloom 开关
- `Exposure`：曝光
- `Threshold`：亮部提取阈值
- `Strength`：Bloom 强度
- `Blur Passes`：模糊次数

建议：

```text
Exposure = 1.0
Threshold = 0.8 到 1.2
Strength = 0.5 到 1.0
Blur Passes = 5 到 8
```

Bloom 主要作用于实时视口。Emissive 物体和高亮 Area Light 更容易触发 Bloom。

## 17. 高质量 CPU 渲染

点击：

```text
High Quality Render
```

渲染开始后：

- 状态栏显示当前渲染状态和进度
- 图片会渐进式更新
- 可以点击 `Stop` 停止
- Stop 后保留已渲染出的结果

CPU 渲染使用当前 Scene 的快照，因此拖动物体或修改参数不会直接破坏正在运行的渲染线程。

CPU 渲染中：

- Point Light 产生硬阴影
- Area Light 产生软阴影
- Emissive 物体会自己发光
- Metal / Glass 会递归反射或折射

## 18. 渲染参数

右侧 `Render Settings` 包含：

- `Width`：输出图片宽度，默认 `800`
- `Height`：输出图片高度，默认 `450`
- `Samples`：每像素采样数，可选 `1 / 4 / 8 / 16 / 32 / 64`
- `Max Depth`：反射和折射递归深度，默认 `5`
- `Threads`：CPU 渲染线程数，默认使用硬件并发线程数

推荐课堂展示参数：

```text
Width = 800
Height = 450
Samples = 16
Max Depth = 5
Threads = 默认
Area Samples = 8 或 16
```

如果渲染太慢，可以降低 `Samples` 或 `Area Samples`。

## 19. 保存图片

点击：

```text
Save Image
```

或菜单：

```text
File -> Save Image
```

保存规则：

- 如果已经生成 CPU 高质量渲染结果，优先保存该结果。
- 如果还没有 CPU 渲染结果，则保存当前实时视口截图。

实时视口截图会包含当前 Bloom 效果。

## 20. 保存场景

点击：

```text
Save Scene
```

或菜单：

```text
File -> Save Scene
```

可以保存当前场景为 JSON 文件。JSON 中包含：

- camera
- render settings
- objects
- materials
- lights
- bloom settings
- soft shadow settings

Area Light 会保存：

- type
- position
- normal
- width
- height
- color
- intensity

Emissive 材质会保存：

- material type
- emissionColor
- emissionStrength

## 21. 加载场景

点击：

```text
Load Scene
```

或菜单：

```text
File -> Load Scene
```

选择之前保存的 JSON 文件即可恢复场景。旧场景文件如果缺少 Area Light、Bloom 或 Emissive 参数，会使用默认值。

## 22. 菜单栏说明

### File

- `Load Scene`：加载 JSON 场景
- `Save Scene`：保存当前场景为 JSON
- `Save Image`：保存当前图像为 PNG
- `Exit`：退出程序

### Render

- `Start Render` 或 `High Quality Render`：启动 CPU 高质量渲染
- `Stop Render`：停止渲染
- `Clear Image`：清空当前高质量渲染结果

### Help

- `About`：查看项目说明

## 23. 推荐展示流程

课堂或答辩展示时，可以按下面顺序：

1. 打开程序，展示中央实时渲染视口。
2. 旋转、平移、缩放视角。
3. 拖动 Sphere 或 Box，说明实时交互不会启动 CPU 渲染。
4. 拖动 Area Light，展示面光源位置变化。
5. 调整 `Area Samples`，说明采样数和软阴影质量的关系。
6. 将一个物体改成 `Emissive`，调整发光颜色和强度。
7. 调整 Bloom 的 `Threshold`、`Strength` 和 `Exposure`。
8. 点击 `High Quality Render`，展示 CPU 光追、Area Light 软阴影和渐进式更新。
9. 点击 `Save Image` 导出 PNG。
10. 点击 `Save Scene` 保存 JSON，再用 `Load Scene` 加载回来。

## 24. 常见问题

### 为什么实时视口和 CPU 渲染结果不完全一样？

实时视口优先保证交互流畅，使用 GPU / OpenGL 快速显示；CPU Ray Tracing 用于最终高质量输出。两者目标不同，因此效果可以有差异。

### 为什么实时视口没有明显软阴影？

当前软阴影重点实现于 CPU 高质量渲染。实时视口中 Area Light 主要负责可视化和基础照明，方便交互。

### 为什么 Area Light 采样数越高越慢？

Area Light 软阴影通过多条 shadow ray 采样矩形面光源。采样越多，阴影越平滑，但 CPU 计算量也越大。

### 为什么 Emissive 物体和 Area Light 都会发亮？

Emissive 是材质层面的自发光物体；Area Light 是显式光源对象。它们看起来都亮，但用途不同。

### 为什么图片有噪点？

提高 `Samples` 和 `Area Samples` 可以减少噪点，但渲染时间会增加。展示时可以先用低参数快速演示，再提高参数渲染最终图。
