# TinyRay Studio 使用文档

本文档面向第一次使用 TinyRay Studio 的用户，按“安装环境、启动程序、渲染图片、编辑场景、保存结果”的顺序说明。

## 1. 软件简介

TinyRay Studio 是一个基于 **C++17 + Qt 6 Widgets + CMake** 的 CPU 光线追踪渲染器 GUI。

它不是游戏，而是一个计算机图形学展示工具。用户可以通过图形界面调整场景、材质、光源和渲染参数，然后点击 `Render` 生成光线追踪图像。

当前支持的图形学功能包括：

- 球体和平面求交
- Diffuse 漫反射材质
- Metal 金属反射材质
- Glass / Dielectric 折射材质
- 点光源直接光照
- 阴影射线
- 递归反射和折射
- Schlick 近似反射概率
- 多采样抗锯齿
- Gamma 校正
- 多线程 CPU 渲染
- 渐进式预览
- PNG 图片导出
- JSON 场景保存和加载

## 2. 运行前准备

如果已经拿到可以运行的 `TinyRayStudio.exe`，并且同目录下已经有 Qt DLL，可以直接双击运行。

如果需要自己编译，需要安装：

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
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.11.0/msvc2022_64"
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

如果双击程序提示缺少 Qt DLL，可以运行：

```powershell
& "C:\Qt\6.11.0\msvc2022_64\bin\windeployqt.exe" "D:\NKUcpp大作业\build\Release\TinyRayStudio.exe"
```

完成后再次双击 `TinyRayStudio.exe`。

如果出现以下 warning，一般可以先忽略：

```text
Warning: Cannot find any version of the dxcompiler.dll and dxil.dll.
Warning: Cannot find Visual Studio installation directory, VCINSTALLDIR is not set.
```

TinyRay Studio 是 Qt Widgets 程序，不依赖 DirectX 渲染管线。

## 5. 主界面说明

程序打开后主要分为三部分：

1. 中央渲染预览区  
   显示当前渲染图像。渲染过程中会渐进式更新。

2. 右侧控制面板  
   包含渲染参数、预设场景、场景对象列表、属性编辑器和操作按钮。

3. 底部状态栏  
   显示当前渲染状态、采样进度、总进度和已用时间。

窗口可以拖动边缘自行调整大小。窗口较小时，右侧控制面板可以滚动。

## 6. 渲染参数说明

右侧 `Render Settings` 区域包含：

- `Width`  
  输出图片宽度，默认 `800`。

- `Height`  
  输出图片高度，默认 `450`。

- `Samples`  
  每像素采样数，可选 `1 / 4 / 8 / 16 / 32 / 64`。  
  数值越高，噪点越少，但渲染越慢。课堂展示推荐 `16`。

- `Max Depth`  
  递归深度，用于控制反射和折射最多递归多少次。默认 `5`。

- `Threads`  
  CPU 渲染线程数。默认使用硬件并发线程数。线程越多通常越快，但也会占用更多 CPU。

## 7. 使用预设场景

右侧 `Presets` 下拉框提供内置场景：

- `Colored Lights Demo`  
  多个彩色点光源照射不同材质球体，适合展示彩色光照和阴影。

- `Reflection Demo`  
  金属球、彩色球和地面，用于展示金属反射。

- `Glass Demo`  
  玻璃球、地面和多光源，用于展示折射、反射和全反射效果。

选择预设后，当前场景会被替换，对象列表会自动刷新。

## 8. 开始渲染

设置好场景和参数后，点击：

```text
Render
```

渲染开始后：

- 中央图像会逐步变清晰；
- 状态栏会显示类似：

```text
Sample 3/16 | 42% | 1.4s
```

含义：

- `Sample 3/16`：当前正在显示第 3 次采样后的结果，总共 16 次采样；
- `42%`：整体渲染进度；
- `1.4s`：已用时间。

渲染完成后，状态栏显示：

```text
Done
```

## 9. 停止渲染

渲染过程中可以点击：

```text
Stop
```

程序会尽快停止当前渲染，并保留已经渲染出来的半成品图像。

停止后可以：

- 保存当前半成品图片；
- 修改参数后重新点击 `Render`；
- 切换预设或编辑场景。

## 10. 保存图片

点击：

```text
Save Image
```

或菜单：

```text
File -> Save Image
```

可以选择路径并保存 PNG 图片。

注意：

- 渲染完成后可以保存最终图；
- 渲染过程中或 Stop 后也可以保存当前预览图。

## 11. 场景对象列表

右侧 `Scene` 区域显示当前场景中的对象和光源。

常见条目：

- `Sphere 1 (Diffuse)`
- `Sphere 2 (Metal)`
- `Sphere 3 (Glass)`
- `Plane 1 (Diffuse)`
- `Point Light 1`

点击列表中的条目后，下方 `Inspector` 会显示该对象的可编辑属性。

## 12. 添加对象

右侧 `Scene` 区域有三个添加按钮：

- `Sphere`  
  添加一个球体。

- `Plane`  
  添加一个平面。

- `Light`  
  添加一个点光源。

添加后，对象会出现在列表中。修改参数后，再次点击 `Render` 即可使用新场景渲染。

## 13. 删除对象

在对象列表中选中一个对象或光源，然后点击：

```text
Delete
```

该对象会从当前场景中删除。

如果误删，可以重新选择预设场景恢复一个完整场景。

## 14. 编辑 Sphere

选中球体后，可以编辑：

- `Center X / Y / Z`  
  球心坐标。

- `Radius`  
  球半径。

- `Material`  
  材质类型：
  - `Diffuse`：漫反射材质；
  - `Metal`：金属反射材质；
  - `Glass`：玻璃折射材质。

- `Albedo`  
  材质基础颜色。

- `Roughness`  
  金属材质粗糙度。  
  越接近 `0`，反射越清晰；越接近 `1`，反射越模糊。

- `Refractive Index`  
  玻璃折射率。常见玻璃可使用 `1.5`。

修改后重新点击 `Render` 查看结果。

## 15. 编辑 Plane

选中平面后，可以编辑：

- `Point X / Y / Z`  
  平面经过的点。

- `Normal X / Y / Z`  
  平面法线方向。  
  例如地面通常使用：

```text
Normal = (0, 1, 0)
```

- `Material`
- `Albedo`
- `Roughness`
- `Refractive Index`

一般情况下，平面建议使用 `Diffuse` 或 `Metal`。

## 16. 编辑 Point Light

选中点光源后，可以编辑：

- `Position X / Y / Z`  
  光源位置。

- `Color`  
  光源颜色。

- `Intensity`  
  光源强度。

如果画面太暗，可以提高 `Intensity`。  
如果画面过曝，可以降低 `Intensity`。

## 17. 保存场景

点击：

```text
Save Scene
```

或菜单：

```text
File -> Save Scene
```

可以保存当前场景为 JSON 文件。

JSON 中包含：

- camera
- render settings
- objects
- materials
- lights

建议文件名示例：

```text
my_scene.json
```

## 18. 加载场景

点击：

```text
Load Scene
```

或菜单：

```text
File -> Load Scene
```

选择之前保存的 JSON 文件即可恢复场景。

如果 JSON 文件格式错误，程序会弹出错误提示，不会崩溃。

## 19. 菜单栏说明

### File

- `Load Scene`：加载 JSON 场景。
- `Save Scene`：保存当前场景为 JSON。
- `Save Image`：保存当前渲染图为 PNG。
- `Exit`：退出程序。

### Render

- `Start Render`：开始渲染。
- `Stop Render`：停止渲染。
- `Clear Image`：清空预览图。

### Help

- `About`：查看项目说明。

## 20. 推荐展示流程

课堂或答辩展示时，可以按下面顺序操作：

1. 打开程序，展示整体界面。
2. 选择 `Colored Lights Demo`。
3. 保持默认参数：

```text
800 x 450
Samples = 16
Max Depth = 5
```

4. 点击 `Render`，展示渐进式渲染和多线程进度。
5. 点击 `Stop`，说明可以保留半成品。
6. 再次点击 `Render` 渲染完成。
7. 切换到 `Reflection Demo`，展示金属反射。
8. 切换到 `Glass Demo`，展示玻璃折射。
9. 选中一个球体，修改颜色或材质，重新渲染。
10. 点击 `Save Image` 导出 PNG。
11. 点击 `Save Scene` 保存 JSON，再用 `Load Scene` 加载回来。

## 21. 参数建议

如果电脑性能一般：

```text
Width = 800
Height = 450
Samples = 8 或 16
Max Depth = 5
Threads = 默认
```

如果想要更清晰的图：

```text
Width = 1280
Height = 720
Samples = 32 或 64
Max Depth = 8
Threads = 默认
```

如果渲染太慢：

- 降低 Samples；
- 降低分辨率；
- 降低 Max Depth；
- 减少玻璃或金属球数量。

## 22. 常见问题

### 程序打不开，提示缺 DLL

运行：

```powershell
& "C:\Qt\6.11.0\msvc2022_64\bin\windeployqt.exe" "D:\NKUcpp大作业\build\Release\TinyRayStudio.exe"
```

然后重新打开程序。

### 编译时提示 TinyRayStudio.exe 无法打开

通常是程序还在运行。先关闭窗口，或在任务管理器中结束 `TinyRayStudio.exe`，再重新编译。

### 渲染出来很暗

可以尝试：

- 增加 Light 的 `Intensity`；
- 添加更多点光源；
- 调整光源位置；
- 换用更亮的 Albedo 颜色。

### 渲染出来噪点很多

增加 Samples：

```text
16 -> 32 -> 64
```

但采样越高，渲染越慢。

### 玻璃球看起来不明显

可以尝试：

- 使用 `Glass Demo`；
- 提高 `Max Depth` 到 `6` 或 `8`；
- 在玻璃球后面放彩色球或彩色光源；
- 使用 `Refractive Index = 1.5`。

### 改了代码但界面没变化

需要重新编译：

```powershell
cmake --build build --config Release
```

然后重新打开程序。

## 23. 文件说明

常用文件：

- `README.md`：项目简介。
- `USER_GUIDE.md`：本文档，详细使用说明。
- `PROJECT_SPEC.md`：完整项目规格说明。
- `AGENTS.md`：Agent 工作规则。
- `VALIDATION_PLAN.md`：验证计划。
- `VALIDATION_REPORT.md`：验证报告。
- `CMakeLists.txt`：CMake 构建配置。
- `src/core/`：光线追踪核心代码。
- `src/gui/`：Qt Widgets GUI 代码。
