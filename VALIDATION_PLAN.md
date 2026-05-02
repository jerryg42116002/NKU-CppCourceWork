# TinyRay Studio 验证计划

本计划供 Validation Agent 使用。Implementation Agent 本轮不运行命令。

## 环境

- Windows + Visual Studio 2022 MSVC
- CMake
- Qt 6 Widgets / OpenGLWidgets

如果 Qt 不在默认搜索路径中，配置时传入 `CMAKE_PREFIX_PATH`，例如：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.11.0\msvc2022_64"
```

## 命令

1. 配置项目：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.11.0\msvc2022_64"
```

2. 构建 Release：

```powershell
cmake --build build --config Release
```

3. 运行非 GUI 自检：

```powershell
.\build\Release\TinyRayStudio.exe --self-test
```

该自检应覆盖 OrbitCamera Turntable：yaw 更新、零速度不旋转、顺/逆时针方向、yaw wrap、Selected Object 目标缺失时回退 Scene Center，以及相机状态不产生 NaN。

## 人工检查

- 默认启动后 Turntable 处于关闭状态。
- 点击 `Start Turntable` 后，实时视口开始围绕目标自动旋转，状态栏显示 `Turntable started`。
- 点击 `Stop Turntable` 后，自动旋转停止，状态栏显示 `Turntable stopped`。
- 调整 `Turntable Speed` 滑块后，旋转速度实时变化；速度设为 0 时画面不再继续转动。
- `Direction` 选择 `Clockwise` / `Counterclockwise` 时，旋转方向相反。
- `Target Mode = Scene Center` 时，相机围绕当前场景中心旋转。
- `Target Mode = Selected Object` 时，选中 Sphere / Box / Cylinder 后相机围绕该物体旋转；没有选中物体时应安全回退到场景中心。
- Turntable 运行时，左键旋转视角、右键平移、滚轮缩放、拖动物体或光源都会暂停 Turntable，状态栏显示 `Turntable paused by user input`。
- 拖动物体后再次点击 `Start Turntable`，若目标模式为 `Selected Object`，旋转目标应使用物体的新位置。
- Turntable 过程中 Overlay Label 应继续跟随选中物体或光源更新。
- Turntable 过程中 Bloom、Skybox / Environment、Texture Mapping 仍正常显示。
- Turntable 过程中点击 `High Quality Render`，Turntable 应先停止，CPU 渲染使用当前实时视口相机姿态的快照，且不会由 Turntable 自动反复触发 CPU 渲染。
- `Reset View` 能恢复默认实时视角。
- `Focus Selected Object` 能把相机目标切到当前选中物体；没有选中物体时显示友好状态。
- 默认场景中能看到至少一个橙色发光球和一个蓝紫色发光物体。
- 右侧 Inspector 中，Sphere / Box / Cylinder / Plane 的材质类型包含 `Emissive`。
- 选择 `Emissive` 后，可以修改 `Emission Color` 和 `Emission Strength`。
- 实时视口中 Emissive 物体即使在普通点光源较弱时也明显可见。
- 点击 High Quality Render 后，CPU Ray Tracing 命中 Emissive 物体时物体自身发亮。
- Save Scene / Load Scene 能保存和恢复 Emissive 材质的类型、发光颜色和发光强度。
