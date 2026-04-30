# VALIDATION_PLAN

本轮是 Implementation Agent：只写代码，不运行命令。以下验证计划供 Validation Agent 在本地 Qt 6 + CMake 环境执行。

## Task Summary

新增详细中文使用文档：

- 新增 `USER_GUIDE.md`，覆盖环境配置、编译、启动、界面、渲染、场景编辑、预设、保存加载、图片导出和常见问题。
- `README.md` 增加到 `USER_GUIDE.md` 的入口链接。

## Changed Files

- `src/gui/MainWindow.cpp`
- `src/gui/RenderWidget.cpp`
- `VALIDATION_PLAN.md`
- `README.md`
- `USER_GUIDE.md`

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

1. 打开 `README.md`，确认“使用方法”下方包含 `USER_GUIDE.md` 链接。
2. 打开 `USER_GUIDE.md`，确认文档包含：
   - 软件简介
   - 运行前准备
   - 编译项目
   - Qt DLL 部署
   - 主界面说明
   - 渲染参数说明
   - 预设场景说明
   - Render / Stop / Save Image 使用说明
   - Sphere / Plane / Point Light 编辑说明
   - Save Scene / Load Scene 说明
   - 菜单栏说明
   - 推荐展示流程
   - 常见问题

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

- 右侧面板改为 `QScrollArea` 后，需要确认所有控件在小窗口下仍能正常点击。
- 默认窗口变小后，需要确认菜单栏、状态栏和 RenderWidget 不会发生遮挡。
- Dock 最小宽度降低后，需要确认 ScenePanel 内按钮文字不会严重挤压。

## Logs For VALIDATION_REPORT.md

Validation Agent 应复制：

- CMake configure 完整输出
- Build 完整输出
- Runtime 手动验证结果摘要
- 如果失败，复制首个关键错误和 suspected cause
