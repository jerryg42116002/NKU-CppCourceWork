# VALIDATION_PLAN

本轮是 Implementation Agent：只写代码，不运行命令。以下验证计划供 Validation Agent 在本地 Qt 6 + CMake 环境执行。

## Task Summary

实现最终可展示版本功能：

- 基础 Scene Panel 场景编辑 GUI
- Sphere / Plane / Point Light 添加、删除、属性编辑
- 内置预设场景
- JSON 场景保存和加载
- 菜单栏 File / Render / Help
- About 窗口
- README 展示性说明更新

## Changed Files

- `CMakeLists.txt`
- `README.md`
- `PROJECT_SPEC.md`
- `src/core/Object.h`
- `src/core/Sphere.h`
- `src/core/Sphere.cpp`
- `src/core/Plane.h`
- `src/core/Plane.cpp`
- `src/core/Scene.h`
- `src/core/Scene.cpp`
- `src/core/SceneIO.h`
- `src/core/SceneIO.cpp`
- `src/gui/MainWindow.h`
- `src/gui/MainWindow.cpp`
- `src/gui/ScenePanel.h`
- `src/gui/ScenePanel.cpp`

## Build Commands

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="你的 Qt 6 Kit 路径"
cmake --build build --config Release
```

如果 Qt 6 已经加入环境变量，也可以执行：

```powershell
cmake -S . -B build
cmake --build build --config Release
```

## Expected Build Result

- CMake configure 成功。
- Release 构建成功。
- 无 C++ 编译错误。
- 无 Qt MOC 错误。
- 无链接错误。

## Manual Runtime Validation

1. 启动 TinyRay Studio。
2. 确认默认窗口大小约为 `1280x720`。
3. 确认菜单栏包含：
   - File / Load Scene / Save Scene / Save Image / Exit
   - Render / Start Render / Stop Render / Clear Image
   - Help / About
4. 点击 Help / About，确认说明包含：
   - TinyRay Studio
   - C++17 + Qt Widgets + CPU Ray Tracing
   - 阴影、反射、折射、抗锯齿、多线程等能力
5. 确认右侧面板包含：
   - Render Settings
   - Presets
   - Scene 对象列表
   - Inspector 属性编辑区
   - Render / Stop / Save Image / Save Scene / Load Scene / Clear

## Scene Editing Validation

1. 在 Scene Panel 中确认当前对象列表显示 Sphere、Plane、Point Light。
2. 点击 `Sphere` 添加球体。
3. 点击 `Plane` 添加平面。
4. 点击 `Light` 添加点光源。
5. 选中新增 Sphere，修改：
   - center x/y/z
   - radius
   - material type
   - albedo color
   - roughness
   - refractive index
6. 选中 Plane，修改：
   - point
   - normal
   - material
7. 选中 Point Light，修改：
   - position
   - color
   - intensity
8. 删除选中对象，确认列表刷新且程序不崩溃。
9. 修改对象后点击 Render，确认最新场景参数参与渲染。

## Preset Validation

1. 在 Presets 下拉框依次选择：
   - Reflection Demo
   - Glass Demo
   - Colored Lights Demo
2. 每次选择后确认对象列表自动替换。
3. 每个预设点击 Render，确认视觉效果明显不同。
4. 默认启动时应加载 Colored Lights Demo。

## Scene Save / Load Validation

1. 修改当前场景中的至少一个对象和一个光源。
2. 点击 `Save Scene`，保存为 JSON 文件。
3. 点击 `Clear` 或切换预设。
4. 点击 `Load Scene` 加载刚才保存的 JSON。
5. 确认：
   - 场景对象恢复；
   - 材质类型恢复；
   - 光源恢复；
   - render settings 恢复；
   - 加载失败时弹出 QMessageBox，不崩溃。

## Render / Progressive / Stop Validation

1. 使用默认设置：
   - 800x450
   - samples 16
   - max depth 5
2. 点击 Render，确认状态栏显示当前状态。
3. 确认 progressive preview 会逐步更新。
4. 渲染过程中点击 Stop，确认保留当前半成品图像。
5. 渲染完成后确认状态栏显示 Done。
6. 点击 Save Image，确认可以选择路径保存 PNG。

## Known Risks

- 本轮新增了大量 Qt Widgets UI 和 JSON 代码，需要重点检查 MOC、信号槽、头文件包含和链接。
- `ScenePanel` 通过场景副本编辑并向 `MainWindow` 发出 `sceneChanged`，需要确认选中项刷新后不会丢失编辑状态。
- JSON 文件格式是当前硬编码 v1，后续如果结构变化需要兼容处理。

## Logs For VALIDATION_REPORT.md

Validation Agent 应复制：

- CMake configure 完整输出
- Build 完整输出
- Runtime 手动验证结果摘要
- 如果失败，复制首个关键错误和 suspected cause
