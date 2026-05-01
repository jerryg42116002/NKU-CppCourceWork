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

## 人工检查

- 默认场景中能看到至少一个橙色发光球和一个蓝紫色发光物体。
- 右侧 Inspector 中，Sphere / Box / Cylinder / Plane 的材质类型包含 `Emissive`。
- 选择 `Emissive` 后，可以修改 `Emission Color` 和 `Emission Strength`。
- 实时视口中 Emissive 物体即使在普通点光源较弱时也明显可见。
- 点击 High Quality Render 后，CPU Ray Tracing 命中 Emissive 物体时物体自身发亮。
- Save Scene / Load Scene 能保存和恢复 Emissive 材质的类型、发光颜色和发光强度。
