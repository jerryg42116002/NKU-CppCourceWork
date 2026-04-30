# VALIDATION_PLAN

本轮调整了根目录 Agent/项目说明文档结构。以下是在本地 Qt 6 + CMake 环境中建议执行的验证计划。

## 文档结构验证

1. 确认根目录包含：
   - `AGENTS.md`
   - `PROJECT_SPEC.md`
   - `VALIDATION_PLAN.md`
   - `VALIDATION_REPORT.md`
2. 确认 `AGENT.md` 不再作为主规则文件使用。
3. 确认 `AGENTS.md` 是短版硬规则。
4. 确认完整项目说明已移入 `PROJECT_SPEC.md`。

## 构建验证

1. 配置 CMake：
   ```powershell
   cmake -S . -B build -DCMAKE_PREFIX_PATH="你的 Qt 6 Kit 路径"
   ```
2. 编译项目：
   ```powershell
   cmake --build build
   ```
3. 确认没有 C++ 编译错误、Qt MOC 错误或链接错误。

## Progressive Rendering 验证

1. 启动 TinyRay Studio。
2. 将 Samples 设置为 `16` 或 `64`，点击 `Render`。
3. 确认渲染过程中中央 `RenderWidget` 不需要等待最终完成，会持续显示当前 `QImage`。
4. 确认预览刷新不是每个像素都刷新，主观上 GUI 仍能响应。
5. 确认状态栏显示：
   - 当前采样数，例如 `Sample 3/16`；
   - 当前进度，例如 `42%`；
   - 已用时间，例如 `1.4s`。
6. 渲染完成后确认状态栏显示 `Done`，进度条为 `100%`。

## Stop 与半成品图像验证

1. 设置较高分辨率，例如 `1600 x 900`。
2. 设置 Samples 为 `64`。
3. 点击 `Render` 后等待预览出现，再点击 `Stop`。
4. 确认：
   - 状态栏先显示 `Stop requested`，随后显示 `Render stopped`；
   - 程序不崩溃、不死锁；
   - 中央预览保留已经渲染出的半成品图像；
   - 可以再次点击 `Render` 启动新渲染。

## Save Image 验证

1. 渲染进行中，等待 progressive preview 出现。
2. 点击 `Save Image`。
3. 确认可以保存当前未完成图像为 PNG。
4. 渲染完成后再次保存，确认保存的是最终图像。

## 多线程与线程安全验证

1. 分别设置 Threads 为 `1`、`2`、`4`、硬件线程数。
2. 确认每种设置都能正常渲染并更新进度。
3. 确认工作线程不直接操作 QWidget：
   - 渲染进度通过 Qt queued signal 更新；
   - progressive image 通过 Qt queued signal 更新；
   - `RenderWidget::setImage()` 只在主线程 slot 中调用。
4. 确认 `Stop` 使用 atomic cancellation flag，并能让 worker thread 尽快退出。

## 回归验证

1. Samples per pixel 仍应生效：`1` 噪点较明显，`16/32/64` 更稳定。
2. Gamma correction 后图像不应过曝或全黑。
3. Diffuse、Metal、Glass 材质仍能正常渲染。
4. 默认场景中应能看到地面、球体、阴影、反射和折射。
