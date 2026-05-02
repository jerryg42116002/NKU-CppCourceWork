# TinyRay Studio 使用文档

本文档面向第一次使用 TinyRay Studio 的用户，也适合课堂展示、课程答辩和功能验收时作为操作手册。文档按“安装运行、界面认识、实时交互、场景编辑、相机动画、材质纹理、灯光环境、CPU 高质量渲染、保存导出、常见问题”的顺序说明。

TinyRay Studio 是一个基于 **C++17 + Qt 6 Widgets + OpenGL + CPU Ray Tracing** 的计算机图形学桌面工具。它不是游戏，也不是静态渲染 demo，而是一个可以交互编辑简单 3D 场景、实时预览并输出高质量光线追踪图像的小型图形工具。

## 1. 功能概览

TinyRay Studio 当前包含两套渲染路径：

- 实时视口：使用 Qt `QOpenGLWidget`、OpenGL 和 GLSL 显示当前场景，用于交互、选中、拖拽、材质和灯光快速预览。
- 高质量渲染：使用 CPU Ray Tracing 生成最终图片，用于展示反射、折射、阴影、抗锯齿和递归光线追踪结果。

当前支持的主要功能：

- 实时 OpenGL / GLSL 视口
- Orbit Camera 鼠标视角控制
- Turntable 相机自动旋转
- 物体和光源选中
- Sphere / Plane / Box / Cylinder 几何体
- Point Light 点光源
- Area Light 矩形面光源
- Diffuse / Metal / Glass / Emissive 材质
- Checkerboard 棋盘格纹理
- Image Texture 图片纹理
- Gradient / Solid Color / Image / HDR Image 环境背景
- Overlay Label 选中信息显示
- Bloom 泛光
- Exposure 曝光控制
- Point Light 硬阴影
- Area Light 多采样软阴影
- Metal 递归反射
- Glass 折射、全反射、Schlick 近似
- 多采样抗锯齿
- 多线程 CPU 渲染
- 渐进式渲染预览
- 渲染取消
- PNG 图片导出
- JSON 场景保存和加载
- `--self-test` 非 GUI 自检

## 2. 运行前准备

如果已经有可以运行的 `TinyRayStudio.exe`，并且同目录下已经部署 Qt DLL，可以直接双击运行。

如果需要自己编译，请安装：

- Windows
- Visual Studio 2022 或 Visual Studio Build Tools
- CMake 3.21 或更新版本
- Qt 6 MSVC 2022 64-bit Kit

推荐 Qt Kit 路径示例：

```text
C:\Qt\6.11.0\msvc2022_64
```

项目不使用 CUDA、OptiX、Vulkan、DirectX、Qt WebEngine 或游戏引擎。

## 3. 编译项目

在 PowerShell 中进入项目根目录：

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

如果 Qt 不在示例路径，请把 `CMAKE_PREFIX_PATH` 改成自己的 Qt Kit 目录。该目录下通常应能找到：

```text
lib\cmake\Qt6\Qt6Config.cmake
```

编译完成后，程序一般位于：

```text
D:\NKUcpp大作业\build\Release\TinyRayStudio.exe
```

## 4. 部署 Qt DLL

如果双击程序提示缺少 Qt DLL，可以运行：

```powershell
C:\Qt\6.11.0\msvc2022_64\bin\windeployqt.exe D:\NKUcpp大作业\build\Release\TinyRayStudio.exe
```

完成后再次双击 `TinyRayStudio.exe`。

如果出现 `dxcompiler.dll` 或 `VCINSTALLDIR` 相关 warning，一般可以先忽略。TinyRay Studio 使用 Qt Widgets 和 OpenGL，不依赖 DirectX 渲染管线。

## 5. 非 GUI 自检

程序支持一个命令行自检入口：

```powershell
.\build\Release\TinyRayStudio.exe --self-test
```

自检覆盖相机、拾取、几何求交、材质、发光、环境、纹理、Area Light、Turntable 等基础逻辑。它不启动 GUI，适合构建后快速确认核心算法没有明显断裂。

## 6. 主界面组成

程序打开后，主要分为四个区域。

### 中央实时视口

中央区域显示当前 3D 场景。它用于：

- 观察场景
- 鼠标旋转、平移和缩放视角
- 点击选中物体或光源
- 拖动物体或光源
- 预览材质、纹理、环境和 Bloom
- 显示 Overlay Label
- 展示 Turntable 自动旋转

实时视口优先保证交互流畅，所以它和 CPU 高质量渲染的结果不要求完全一致。

### 右侧控制面板

右侧面板包含多个分组：

- `Render Settings`：CPU 渲染参数和拖拽模式
- `Camera`：Turntable、Reset View、Focus Selected Object
- `Presets`：内置场景
- `Environment`：背景环境
- `Overlay`：选中信息标签
- `Post Processing`：Bloom 和曝光
- `Shadows`：软阴影采样
- `Scene`：对象和光源列表
- `Inspector`：选中对象或光源的属性编辑器
- `Output`：渲染、停止、保存、加载、清空

右侧内容较多，可以滚动查看。

### 菜单栏

菜单栏包含：

- `File`
- `Render`
- `Help`

常用操作也可以通过右侧按钮完成。

### 状态栏

底部状态栏显示当前状态，例如：

- `Real-time rendering`
- `Camera orbiting`
- `Object dragging`
- `Turntable started`
- `Turntable stopped`
- `Turntable paused by user input`
- `High quality rendering`
- `Render done`

右侧进度条用于显示 CPU 高质量渲染进度。

## 7. 实时视口鼠标操作

常用鼠标操作如下：

- 左键拖动空白区域：旋转视角
- 右键拖动：平移视角
- 鼠标滚轮：缩放视角
- 左键点击物体：选中物体
- 左键拖动物体：移动物体
- 左键点击光源：选中光源
- 左键拖动光源：移动光源

当前支持拖拽的对象：

- Sphere
- Box
- Cylinder
- Point Light
- Area Light

Plane 通常作为地面或背景平面使用，不建议用鼠标拖动。可以在 Inspector 中精确修改 Plane 的位置和法线。

## 8. 拖拽模式

右侧 `Render Settings` 中的 `Drag Mode` 控制鼠标拖拽物体或光源时的移动约束。

可选模式：

- `Move XZ Plane`：在地面平面上移动，适合摆放物体。
- `Move XY Plane`：在 X/Y 平面移动，适合正面视角下调整高度和水平位置。
- `Move YZ Plane`：在 Y/Z 平面移动，适合侧向布置。
- `Move X Axis`：只改变 X 坐标。
- `Move Y Axis`：只改变高度。
- `Move Z Axis`：只改变 Z 坐标。

推荐用法：

- 摆放球体、盒子、圆柱体：优先使用 `Move XZ Plane`。
- 调整光源高度：使用 `Move Y Axis`。
- 需要精确输入时：直接在 Inspector 中修改数值。

## 9. Camera 面板

`Camera` 面板用于控制实时视口相机和展示辅助功能。

### Start Turntable

点击 `Start Turntable` 后，相机会自动围绕目标旋转。这个功能适合：

- 课堂展示
- 录屏答辩
- 展示 Bloom、反射、玻璃和环境效果
- 快速观察模型在不同角度下的外观

Turntable 默认关闭，需要手动启动。

### Stop Turntable

点击 `Stop Turntable` 后，相机停止自动旋转。

### Speed

`Speed` 控制旋转速度，单位为 degrees / second。数值越大，旋转越快。

推荐值：

```text
15 到 30 degrees / second
```

如果速度设为 `0`，Turntable 保持开启状态也不会继续旋转。

### Direction

`Direction` 控制旋转方向：

- `Clockwise`：顺时针
- `Counterclockwise`：逆时针

方向切换会实时生效。

### Target Mode

`Target Mode` 控制 Turntable 围绕什么位置旋转：

- `Scene Center`：围绕当前场景中心旋转。
- `Selected Object`：围绕当前选中物体旋转。
- `Custom Target`：使用当前相机目标作为自定义目标。

`Selected Object` 模式下，如果没有选中物体，程序会自动回退到场景中心，不会崩溃。

如果正在围绕某个物体旋转，然后拖动该物体，目标位置会同步到物体的新位置。拖动本身会暂停 Turntable，用户可以再次点击 `Start Turntable` 继续。

### Reset View

`Reset View` 用于恢复默认实时视角。适合视角转乱、缩放过近或找不到场景时使用。

### Focus Selected Object

`Focus Selected Object` 会把相机目标切到当前选中物体，方便围绕该物体观察。

如果没有选中物体，状态栏会提示没有可聚焦对象。

### Turntable 与鼠标交互

为了避免自动旋转和手动操作互相抢控制权，以下操作会自动暂停 Turntable：

- 左键拖动旋转视角
- 右键拖动平移视角
- 鼠标滚轮缩放
- 拖动物体
- 拖动光源

暂停后状态栏显示：

```text
Turntable paused by user input
```

用户可以再次点击 `Start Turntable` 继续自动旋转。

### Turntable 与 CPU 渲染

Turntable 只影响实时视口，不会自动触发 CPU Ray Tracing。

如果 Turntable 正在运行时点击 `High Quality Render`，程序会先停止 Turntable，然后使用当前实时视口的相机姿态作为 CPU 渲染快照。这意味着可以先用 Turntable 找角度，再渲染最终图。

## 10. 预设场景

右侧 `Presets` 下拉框提供内置场景。

### Colored Lights Demo

默认场景，展示内容最丰富：

- 彩色 Point Light
- Area Light
- Metal
- Glass
- Emissive 发光物体
- Bloom
- Checkerboard 地面
- 环境背景

适合综合展示项目功能。

### Reflection Demo

主要展示：

- 金属反射
- 彩色 diffuse 物体
- 地面反射和基础光照
- 多种几何体组合

适合讲解 Metal 材质和递归反射。

### Glass Demo

主要展示：

- 玻璃折射
- 全反射
- Schlick 近似
- 多光源照明

适合讲解 Glass / Dielectric 材质。

选择预设后，当前场景会被替换，对象列表会刷新，高质量渲染结果会清空。

## 11. Scene 对象列表

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

点击条目后，下方 `Inspector` 会显示该对象或光源的属性。选择对象后，实时视口中该对象会高亮，并且 Overlay Label 会显示对象信息。

## 12. 添加和删除对象

`Scene` 区域下方有添加按钮：

- `Sphere`：添加球体
- `Box`：添加盒子
- `Cyl`：添加圆柱
- `Plane`：添加平面
- `Point`：添加点光源
- `Area`：添加矩形面光源
- `Delete`：删除当前选中对象或光源

删除对象后，如果 Turntable 正在使用 `Selected Object` 模式，目标会安全回退到场景中心。

## 13. Inspector：编辑 Sphere

选中 Sphere 后，可以编辑：

- `Center X / Y / Z`：球心位置
- `Radius`：半径
- `Material`：材质类型
- `Albedo`：基础颜色
- `Texture`：是否启用纹理
- `Texture Path`：加载图片纹理
- `Texture Scale`：纹理缩放
- `Texture Offset U / V`：纹理偏移
- `Texture Strength`：纹理混合强度
- `Roughness`：金属粗糙度
- `Refractive Index`：玻璃折射率
- `Emission Color`：发光颜色
- `Emission Strength`：发光强度

修改后实时视口会立即更新。CPU 渲染会在下一次点击 `High Quality Render` 时使用最新参数。

## 14. Inspector：编辑 Box

选中 Box 后，可以编辑：

- `Center X / Y / Z`：中心位置
- `Size X / Y / Z`：长宽高
- `Material`
- `Albedo`
- `Texture`
- `Texture Path`
- `Texture Scale`
- `Texture Offset U / V`
- `Texture Strength`
- `Roughness`
- `Refractive Index`
- `Emission Color`
- `Emission Strength`

Box 适合展示金属块、玻璃块、发光立柱等效果。

## 15. Inspector：编辑 Cylinder

选中 Cylinder 后，可以编辑：

- `Center X / Y / Z`：中心位置
- `Radius`：半径
- `Height`：高度
- `Material`
- `Albedo`
- `Texture`
- `Texture Path`
- `Texture Scale`
- `Texture Offset U / V`
- `Texture Strength`
- `Roughness`
- `Refractive Index`
- `Emission Color`
- `Emission Strength`

Cylinder 适合展示柱体、灯柱和简单工业形体。

## 16. Inspector：编辑 Plane

选中 Plane 后，可以编辑：

- `Point X / Y / Z`：平面经过的点
- `Normal X / Y / Z`：平面法线
- `Material`
- `Albedo`
- `Texture`
- `Texture Path`
- `Texture Scale`
- `Texture Offset U / V`
- `Texture Strength`
- `Roughness`
- `Refractive Index`
- `Emission Color`
- `Emission Strength`

Plane 常用作地面。默认地面适合搭配 Checkerboard 纹理，以便观察透视、反射和阴影。

## 17. 材质类型

### Diffuse

Diffuse 是漫反射材质，适合普通彩色物体和地面。

特点：

- 颜色主要由 `Albedo` 决定
- 受灯光方向和阴影影响明显
- 不产生镜面反射或折射

### Metal

Metal 是金属反射材质。

关键参数：

- `Albedo`：金属颜色
- `Roughness`：粗糙度

`Roughness` 越低，反射越清晰；越高，反射越模糊。推荐展示值：

```text
Roughness = 0.05 到 0.25
```

### Glass

Glass 是玻璃 / Dielectric 材质。

关键参数：

- `Albedo`：透射颜色
- `Refractive Index`：折射率

常用折射率：

```text
1.5
```

CPU Ray Tracing 中，Glass 会计算反射、折射、全反射和 Schlick 近似。

### Emissive

Emissive 是自发光材质。

设置方法：

1. 选中 Sphere、Box、Cylinder 或 Plane。
2. 将 `Material` 改成 `Emissive`。
3. 设置 `Emission Color`。
4. 调整 `Emission Strength`。

推荐值：

```text
Emission Strength = 3.0 到 8.0
```

Emissive 和 Area Light 是两个不同概念：

- Emissive 是物体材质，看起来会自己发光。
- Area Light 是显式光源，会参与软阴影采样。

当前 Emissive 物体可以触发实时 Bloom。CPU Ray Tracing 命中 Emissive 物体时会返回发光颜色。

## 18. 纹理功能

材质可以启用纹理。纹理会和 `Albedo` 按 `Texture Strength` 混合。

### Enable Texture

勾选后启用纹理采样。不勾选时只使用 `Albedo`。

### Texture Path

点击 `Load Texture` 可以选择图片纹理。支持常见图片格式，例如：

- PNG
- JPG / JPEG
- BMP
- TGA

如果没有加载图片，纹理会使用 Checkerboard 棋盘格。

### Texture Scale

控制纹理重复频率。

- 数值越大，棋盘格或图片重复越密。
- 数值越小，纹理显示越大。

推荐值：

```text
Plane: 4 到 8
Sphere: 1 到 3
Box / Cylinder: 1 到 4
```

### Texture Offset U / V

控制纹理在 U/V 方向上的偏移。适合微调图片纹理位置。

### Texture Strength

控制纹理和基础颜色的混合强度。

- `0.0`：只使用 Albedo
- `1.0`：主要使用纹理

推荐先使用 `1.0` 观察纹理，再根据需要降低。

## 19. 编辑 Point Light

选中 Point Light 后，可以编辑：

- `Position X / Y / Z`：点光源位置
- `Color`：光源颜色
- `Intensity`：光源强度

Point Light 在 CPU 渲染中使用单条 shadow ray，因此阴影边缘较硬。

推荐值：

```text
Intensity = 10 到 30
```

如果画面太暗，提高 `Intensity` 或增加光源数量。如果高光过曝，降低 `Intensity` 或降低 Bloom / Exposure。

## 20. 编辑 Area Light

选中 Area Light 后，可以编辑：

- `Position X / Y / Z`：矩形面光源中心位置
- `Normal X / Y / Z`：矩形面朝向
- `Width`：矩形宽度
- `Height`：矩形高度
- `Color`：光源颜色
- `Intensity`：光源强度

实时视口中，Area Light 会显示为发光矩形。它可以被选中和拖拽。

CPU 高质量渲染中，Area Light 会对矩形表面进行多点采样，产生软阴影。

## 21. Shadows 面板

`Shadows` 面板控制 Area Light 软阴影。

- `Enabled`：是否启用软阴影采样
- `Area Samples`：面光源采样数

推荐值：

```text
Area Samples = 8 或 16
```

采样数越高：

- 阴影越平滑
- 噪点越少
- CPU 渲染越慢

展示时可以先用 `8` 快速演示，再用 `16` 或更高值渲染最终图。

## 22. Environment 面板

`Environment` 面板控制背景和 miss ray 颜色。

### Mode

可选模式：

- `Gradient`：上下渐变背景
- `Solid Color`：纯色背景
- `Image`：使用普通图片作为环境贴图
- `HDR Image`：作为 HDR 环境贴图模式处理

如果图片路径无效，程序会安全回退到渐变背景。

### Load Image

点击 `Load Image` 可以选择环境图片。实时视口会使用它作为环境背景和反射参考。

### Exposure

控制环境曝光。数值越高，背景和环境贡献越亮。

### Intensity

控制环境强度。数值越高，环境颜色对画面影响越明显。

### Rotation Y

绕 Y 轴旋转环境贴图。用于调整环境图案或高亮位置。

## 23. Overlay 面板

`Overlay` 面板控制实时视口里的选中标签。

- `Show Overlay Labels`：是否显示标签
- `Show Position`：是否显示位置
- `Show Material Info`：是否显示材质或灯光信息

选中物体时，Overlay 可能显示：

- 对象类型
- 对象 id
- 位置
- 材质类型
- 是否使用纹理
- Emissive 强度

选中光源时，Overlay 可能显示：

- 光源类型
- 位置
- 强度
- Area Light 尺寸

Turntable 运行时，Overlay 会继续跟随目标在屏幕上更新。

## 24. Post Processing 面板

`Post Processing` 面板控制 Bloom 和曝光。

### Enabled

开启或关闭 Bloom。

### Exposure

控制最终显示亮度。实时视口和 CPU 渲染后的 Bloom 后处理都会参考曝光设置。

### Threshold

亮部提取阈值。低于阈值的区域不容易产生 Bloom。

推荐值：

```text
0.8 到 1.2
```

### Strength

Bloom 强度。数值越高，发光扩散越明显。

推荐值：

```text
0.5 到 1.0
```

### Blur Passes

模糊次数。数值越高，Bloom 越柔和，但实时成本也更高。

推荐值：

```text
5 到 8
```

Emissive 物体和高亮 Area Light 最容易触发 Bloom。

## 25. Render Settings 面板

`Render Settings` 面板主要控制 CPU 高质量渲染。

- `Width`：输出图片宽度，默认 `800`
- `Height`：输出图片高度，默认 `450`
- `Samples`：每像素采样数，可选 `1 / 4 / 8 / 16 / 32 / 64`
- `Max Depth`：递归深度，默认 `5`
- `Threads`：渲染线程数
- `Drag Mode`：实时视口拖拽模式

推荐课堂展示参数：

```text
Width = 800
Height = 450
Samples = 16
Max Depth = 5
Threads = 默认
Area Samples = 8 或 16
```

如果渲染太慢：

- 降低 `Samples`
- 降低 `Area Samples`
- 降低输出分辨率
- 降低 `Max Depth`

如果玻璃或金属递归效果不够明显，可以适当提高 `Max Depth`。

## 26. 高质量 CPU 渲染

点击：

```text
High Quality Render
```

渲染开始后：

- 状态栏显示当前 sample、总 sample、进度和耗时
- 进度条更新
- 图像渐进式更新
- 可以点击 `Stop` 停止
- Stop 后保留已经渲染出的结果

CPU 渲染使用当前 Scene 的快照。也就是说，渲染开始后继续拖动物体或修改参数，不会直接破坏正在运行的渲染线程。

CPU Ray Tracing 支持：

- 相机射线生成
- Sphere / Plane / Box / Cylinder 求交
- Diffuse 直接光照
- Point Light 硬阴影
- Area Light 软阴影
- Metal 递归反射
- Glass 折射和全反射
- Emissive 命中发光
- 环境背景 miss ray
- 多采样抗锯齿
- Gamma correction
- 多线程渲染
- 渐进式更新

## 27. Stop 和 Clear

### Stop

点击 `Stop` 可以请求停止当前 CPU 渲染。

停止不是强制杀线程，而是让渲染器安全检查 stop flag 后退出。停止后，已经完成的部分图像会保留。

### Clear

点击 `Clear` 清空当前高质量渲染结果，并把显示状态回到实时渲染状态。

## 28. 保存图片

点击：

```text
Save Image
```

或使用菜单：

```text
File -> Save Image
```

保存规则：

- 如果已经生成 CPU 高质量渲染结果，优先保存 CPU 图像。
- 如果没有 CPU 渲染结果，则保存当前实时视口截图。

实时视口截图会包含当前 Bloom 效果和当前相机视角。

保存格式为 PNG。如果文件名没有后缀，程序会自动补 `.png`。

## 29. 保存场景

点击：

```text
Save Scene
```

或使用菜单：

```text
File -> Save Scene
```

场景会保存为 JSON 文件。保存内容包括：

- camera
- render settings
- objects
- materials
- texture settings
- lights
- bloom settings
- environment settings
- overlay settings
- soft shadow settings

材质会保存：

- material type
- albedo
- roughness
- refractive index
- emission color
- emission strength
- texture path
- texture scale
- texture offset
- texture strength

Area Light 会保存：

- type
- position
- normal
- width
- height
- color
- intensity

## 30. 加载场景

点击：

```text
Load Scene
```

或使用菜单：

```text
File -> Load Scene
```

选择之前保存的 JSON 文件即可恢复场景。

加载规则：

- 加载成功后当前场景会被替换。
- 对象列表和 Inspector 会刷新。
- CPU 高质量渲染结果会清空。
- 旧场景文件如果缺少新字段，会使用默认值。
- 如果环境图片或纹理图片路径失效，会安全回退到默认颜色或渐变背景。

如果当前正在 CPU 渲染，程序会提示先停止渲染再加载场景。

## 31. 菜单栏说明

### File

- `Load Scene`：加载 JSON 场景
- `Save Scene`：保存当前场景为 JSON
- `Save Image`：保存当前图像为 PNG
- `Exit`：退出程序

### Render

- `High Quality Render`：启动 CPU 高质量渲染
- `Stop Render`：停止当前渲染
- `Clear Image`：清空当前高质量渲染结果

### Help

- `About`：查看项目说明

## 32. 推荐课堂展示流程

可以按下面顺序展示：

1. 打开程序，展示 `Colored Lights Demo` 默认场景。
2. 说明中央实时视口和右侧控制面板。
3. 使用鼠标旋转、平移、缩放视角。
4. 点击 `Start Turntable`，展示自动环绕效果。
5. 调整 Turntable `Speed` 和 `Direction`。
6. 选中一个物体，将 `Target Mode` 改为 `Selected Object`，围绕它旋转。
7. 手动拖动视角，展示 Turntable 自动暂停。
8. 拖动 Sphere / Box / Cylinder，说明实时编辑不会触发 CPU 渲染。
9. 拖动 Point Light 或 Area Light，观察光照变化。
10. 打开 Overlay，展示选中对象信息。
11. 修改一个物体的材质为 `Metal`，调整 `Roughness`。
12. 修改一个物体的材质为 `Glass`，说明折射率。
13. 修改一个物体为 `Emissive`，调整发光颜色和强度。
14. 启用或调整 Texture，展示 Checkerboard 或图片纹理。
15. 调整 Environment 的曝光、强度或环境图片。
16. 调整 Bloom 的 `Threshold`、`Strength` 和 `Exposure`。
17. 调整 `Area Samples`，说明采样数和软阴影质量的关系。
18. 点击 `High Quality Render`，展示 CPU 光追渐进式更新。
19. 渲染完成后点击 `Save Image` 导出 PNG。
20. 点击 `Save Scene` 保存 JSON，再用 `Load Scene` 加载回来。

## 33. 推荐参数组合

### 快速演示

```text
Width = 800
Height = 450
Samples = 4 或 8
Max Depth = 4
Area Samples = 4 或 8
```

适合课堂现场快速演示。

### 质量展示

```text
Width = 1280
Height = 720
Samples = 16 或 32
Max Depth = 5 到 8
Area Samples = 16 或 32
```

适合导出最终图片。

### Turntable 录屏

```text
Turntable Speed = 15 到 24 deg/s
Direction = Counterclockwise
Target Mode = Scene Center 或 Selected Object
Bloom Strength = 0.5 到 0.9
Exposure = 1.0 左右
```

录屏时建议先用 `Reset View` 找回默认视角，再用 `Focus Selected Object` 或 `Target Mode` 设置旋转中心。

## 34. 常见问题

### 为什么实时视口和 CPU 渲染结果不完全一样？

实时视口优先保证交互流畅，使用 GPU / OpenGL 快速显示；CPU Ray Tracing 用于最终高质量输出。两者目标不同，因此效果可以有差异。

### 为什么实时视口没有明显软阴影？

软阴影重点体现在 CPU 高质量渲染中。实时视口中的 Area Light 主要用于可视化和快速交互。

### 为什么 Area Light 采样数越高越慢？

Area Light 软阴影需要从 shading point 向矩形面光源多个采样点发射 shadow ray。采样越多，阴影越平滑，但 CPU 计算量也越大。

### 为什么 Emissive 物体和 Area Light 都会发亮？

Emissive 是材质层面的自发光物体；Area Light 是显式光源对象。它们看起来都亮，但用途不同。

### 为什么图片有噪点？

提高 `Samples` 和 `Area Samples` 可以减少噪点，但渲染时间会增加。展示时可以先用低参数快速演示，再提高参数渲染最终图。

### 为什么点击 High Quality Render 后 Turntable 停了？

这是设计行为。CPU 渲染使用当前视口相机姿态作为快照。开始 CPU 渲染前暂停 Turntable，可以避免最终渲染角度在用户预期之外继续变化。

### 为什么鼠标一操作 Turntable 就暂停？

这是为了避免自动旋转和手动操作互相冲突。手动操作后可以再次点击 `Start Turntable` 继续。

### 为什么 Selected Object 模式没有围绕物体旋转？

请确认已经选中 Sphere、Box、Cylinder 或其他可选对象。如果当前选中的是光源或没有选中对象，Turntable 会回退到场景中心。

### 为什么加载图片纹理后看不到明显变化？

可以检查：

- 是否勾选 `Enable Texture`
- `Texture Strength` 是否大于 0
- `Texture Scale` 是否过大或过小
- 图片路径是否有效
- 当前材质颜色是否太暗

### 为什么环境图片加载后 CPU 渲染背景不一样？

环境图片会影响 miss ray 和实时预览，但不同渲染路径的色调映射和采样方式可能不同，因此实时视口和 CPU 输出可以有轻微差异。

### 为什么 CMake 找不到 Qt6？

需要给 CMake 指定 Qt Kit 路径，例如：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.11.0\msvc2022_64"
```

如果 Qt 安装在其他位置，请修改 `CMAKE_PREFIX_PATH`。

## 35. 一句话工作流

最常用流程是：

```text
选择预设 -> 调整视角或 Turntable -> 选中并编辑物体/光源 -> 调整材质/纹理/环境/Bloom -> High Quality Render -> Save Image / Save Scene
```

TinyRay Studio 的实时视口用于快速摆场景和找角度，CPU Ray Tracing 用于最终高质量输出。两者配合使用，才能体现这个项目的完整图形学工作流。
