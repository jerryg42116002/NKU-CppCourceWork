# VALIDATION_PLAN

本轮是 Implementation Agent：只写代码，不运行构建、测试或程序。以下内容供后续 Validation Agent 执行。

## 本轮任务

在 `GLPreviewWidget` 中实现鼠标拖拽球体功能：

- 左键按中球体后选中球体并进入拖拽状态。
- 鼠标移动时，球体沿地面 XZ 平面移动，Y 坐标保持不变。
- 拖动过程中 OpenGL Preview 实时刷新。
- 右侧 Scene Panel 同步显示球体新坐标。
- 点击 `Render` 时，CPU Ray Tracing 使用拖动后的最新 Scene snapshot。

## 变更范围

- `CMakeLists.txt`
- `src/main.cpp`
- `src/gui/GLPreviewWidget.h`
- `src/gui/GLPreviewWidget.cpp`
- `src/gui/MainWindow.h`
- `src/gui/MainWindow.cpp`
- `src/gui/ScenePanel.h`
- `src/gui/ScenePanelSelection.cpp`
- `VALIDATION_PLAN.md`

## 构建验证

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="你的 Qt 6 Kit 路径"
cmake --build build --config Release
```

如果 Qt 6 已加入环境变量，也可以执行：

```powershell
cmake -S . -B build
cmake --build build --config Release
```

期望结果：

- CMake configure 成功。
- Release 构建成功。
- 正确链接 `Qt6::Widgets`、`Qt6::OpenGL`、`Qt6::OpenGLWidgets`。
- 不引入 CUDA、OptiX、Vulkan、DirectX、Qt WebEngine 或游戏引擎依赖。

## Self-Test 验证

构建成功后执行：

```powershell
.\build\Release\TinyRayStudio.exe --self-test
```

期望结果：

- 进程返回码为 `0`。
- `OrbitCamera` 默认状态、orbit、zoom、pan 后均合法且不产生 NaN。
- 对象 id 为稳定的唯一正整数。
- ray 命中 sphere 后，`HitRecord::hitObjectId` 返回正确对象 id。
- ray 未命中时，不会错误返回选中对象。
- ray-plane 求交结果正确。
- sphere center 修改后，`Scene::intersect` 的命中结果发生对应变化。
- Scene snapshot 不受后续 GUI Scene 修改影响。

## 手工运行验证

1. 启动 TinyRay Studio。
2. 切换到 `Preview` 标签页。
3. 左键单击球体：
   - 球体被选中。
   - OpenGL Preview 中选中球体高亮。
   - 状态栏显示选中对象。
   - Scene Panel 选中对应对象并显示其坐标。
4. 左键按住球体并拖动：
   - 球体沿 XZ 平面移动。
   - Y 坐标保持不变。
   - OpenGL Preview 实时刷新。
   - Scene Panel 中 center x / z 实时更新。
5. 左键按住空白区域并拖动：
   - 相机 orbit 旋转仍然正常。
   - 不移动任何球体。
6. 右键拖动：
   - 相机 pan 平移仍然正常。
7. 鼠标滚轮：
   - 相机 zoom 缩放仍然正常。
8. 点击空白区域：
   - 清除选中状态。
9. 拖动球体后点击 `Render`：
   - CPU Ray Tracing 使用球体的新位置渲染。
   - 渲染线程不直接读取正在被 GUI 修改的 Scene。
10. 渲染过程中尝试在 Preview 中拖动球体：
    - 程序不崩溃。
    - 当前 CPU 渲染继续使用启动时的 Scene snapshot。

## 回归验证

1. Scene Panel 手动修改球体坐标后，OpenGL Preview 自动刷新。
2. 删除当前选中球体后，不应崩溃。
3. Plane 和 Point Light 暂时不要求可拖拽，但仍应能正常显示。
4. `Save Image` 仍然保存 CPU Ray Tracing 的 QImage，不保存 OpenGL Preview。
5. `Save Scene` 保存拖动后的球体位置。
6. `Load Scene` 后，OpenGL Preview 与 Scene Panel 显示加载后的场景。

## 记录到 VALIDATION_REPORT.md

Validation Agent 完成后，请记录：

- CMake configure 结果。
- Release build 结果。
- `--self-test` 输出和返回码。
- 球体拖拽的手工验证结果。
- CPU 渲染 snapshot 行为验证结果。
- 如有失败，记录第一个关键错误和 suspected cause。
