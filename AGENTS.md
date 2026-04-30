# AGENTS.md

TinyRay Studio 的每个 Agent 都必须先读本文件。本文件只保留硬规则；完整项目说明在 `PROJECT_SPEC.md`。

## 项目硬约束

- 项目是 C++ 图形学桌面工具，不是游戏。
- 技术栈固定为 C++17、Qt 6 Widgets、CMake、QImage、标准 C++ 线程工具。
- 渲染器必须运行在 CPU 上。
- 禁止使用 CUDA、Vulkan、DirectX、游戏引擎、Web 前端和重型外部依赖。
- OpenGL 不是必需项，除非任务明确要求，否则不要引入。

## 代码硬规则

- 不要把所有逻辑写进 `main.cpp`。
- GUI、渲染器、数学、场景、几何体、材质、相机模块必须保持分离。
- 核心 ray tracing 代码尽量独立于 Qt；Qt 主要用于 GUI、事件、图像显示和文件对话框。
- Worker thread 不得直接操作 QWidget；必须通过 Qt signal/slot 回到主线程更新 UI。
- 渲染取消必须使用线程安全机制，例如 atomic cancellation flag。
- 代码应保持可读、模块化、可扩展，避免全局可变状态。

## Agent 分工硬规则

### Implementation Agent

- 可读写源代码、CMake、README、文档和 `VALIDATION_PLAN.md`。
- 每次实现任务结束必须更新 `VALIDATION_PLAN.md`。
- 如果用户明确要求“只写代码，不运行命令”，不得运行 shell/build/test 命令。
- 不得声称构建、测试或运行已通过，除非 `VALIDATION_REPORT.md` 中有对应证据。

### Validation Agent

- 只执行 `VALIDATION_PLAN.md` 中的验证命令。
- 可创建或更新 `VALIDATION_REPORT.md`。
- 不得修改源代码、CMake、GUI、头文件或项目架构。
- 验证失败时只报告失败信息，不自行修复。

## 何时读取 PROJECT_SPEC.md

- 高推理写代码、架构设计、大功能开发、渲染算法修改、GUI 工作流修改时，先读取 `PROJECT_SPEC.md`。
- 简单 git 操作、简单说明、少量文档调整、验证执行任务，不需要读取 `PROJECT_SPEC.md`。

## 验证文件

- `VALIDATION_PLAN.md`：验证命令和预期结果。
- `VALIDATION_REPORT.md`：实际验证结果和错误摘要。
