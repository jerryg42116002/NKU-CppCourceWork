# VALIDATION_PLAN

本轮是 Implementation Agent：代码已更新，未运行构建、测试或程序。以下内容供后续 Validation Agent 执行。

## 本轮任务

新增物体拖拽坐标轴 / 平面切换功能。

## 新增功能

右侧 Render Settings 面板新增 `Drag Mode` 下拉框：

- `Move XZ Plane`
- `Move XY Plane`
- `Move YZ Plane`
- `Move X Axis`
- `Move Y Axis`
- `Move Z Axis`

拖拽对象支持：

- Sphere
- Box
- Cylinder
- Metal / Glass 版本的 Box 和 Cylinder

## 变更范围

- `CMakeLists.txt`
- `src/interaction/DragMode.h`
- `src/gui/GLPreviewWidget.h`
- `src/gui/GLPreviewWidget.cpp`
- `src/gui/MainWindow.h`
- `src/gui/MainWindow.cpp`
- `VALIDATION_PLAN.md`

## 构建验证

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="你的 Qt 6 Kit 路径"
cmake --build build --config Release
```

如果 Qt 6 和 CMake 已加入环境变量，也可以执行：

```powershell
cmake -S . -B build
cmake --build build --config Release
```

期望结果：

- CMake configure 成功。
- Release 构建成功。
- 不引入 CUDA、OptiX、Vulkan、DirectX、Qt WebEngine 或游戏引擎依赖。

## Self-Test 验证

构建成功后执行：

```powershell
.\build\Release\TinyRayStudio.exe --self-test
```

期望结果：

- 进程返回码为 `0`。
- 现有非 GUI 逻辑测试全部通过。

## 手工运行验证

1. 启动 TinyRay Studio。
2. 确认右侧 Render Settings 中出现 `Drag Mode` 下拉框。
3. 选择 `Move XZ Plane`：
   - 拖动物体时只改变 X / Z，Y 基本保持不变。
4. 选择 `Move XY Plane`：
   - 拖动物体时只改变 X / Y，Z 基本保持不变。
5. 选择 `Move YZ Plane`：
   - 拖动物体时只改变 Y / Z，X 基本保持不变。
6. 选择 `Move X Axis`：
   - 拖动物体时主要沿 X 轴移动。
7. 选择 `Move Y Axis`：
   - 拖动物体时主要沿 Y 轴移动。
8. 选择 `Move Z Axis`：
   - 拖动物体时主要沿 Z 轴移动。
9. 对 Sphere / Box / Cylinder 分别重复上述至少一种拖拽模式。
10. 拖动后点击 `High Quality Render`：
    - CPU Ray Tracing 使用拖动后的最新位置。
11. 拖动后点击 `Save Scene`，再 `Load Scene`：
    - 对象新位置被保存并恢复。

## 已知限制

- 单轴拖拽基于屏幕位移投影，手感是稳定可用的近似 gizmo，不是完整三维变换操纵器。
- 当前没有显示彩色 XYZ 轴 gizmo，可在后续阶段添加。
- Plane 和 PointLight 暂不支持拖拽。

## 记录到 VALIDATION_REPORT.md

Validation Agent 完成后，请记录：

- CMake configure 结果。
- Release build 结果。
- `--self-test` 输出和返回码。
- 六种 Drag Mode 的手工验证结果。
- 如有失败，记录第一个关键错误和 suspected cause。
