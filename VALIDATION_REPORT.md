# VALIDATION_REPORT

## Validation Plan Read

- Read `VALIDATION_PLAN.md`.
- Planned command sequence:
  1. `cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.11.0\msvc2022_64"`
  2. `cmake --build build --config Release`
  3. `.\build\Release\TinyRayStudio.exe --self-test`

## Commands

- `cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.11.0\msvc2022_64"`: FAIL / TIMEOUT
  - Exit code: `124`
  - The command timed out after about 448 seconds.
  - Key output:
    ```text
    -- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.
    -- Found OpenGL: opengl32
    -- Configuring incomplete, errors occurred!
    CMake Error ... could not be removed:
      拒绝访问。
    ```
  - The repeated failing paths were under:
    ```text
    D:/NKUcpp大作业/build/CMakeFiles/CMakeScratch/TryCompile-*/
    ```

- `cmake -S . -B build_validation -DCMAKE_PREFIX_PATH="C:\Qt\6.11.0\msvc2022_64"`: FAIL
  - Exit code: `1`
  - This was an extra isolation attempt to check whether the original `build` directory was locked or stale.
  - Key output:
    ```text
    Failed to run MSBuild command:
      D:/Visual Studio/Visual Studio 2022/Community/MSBuild/Current/Bin/amd64/MSBuild.exe
    to get the value of VCTargetsPath

    error MSB3061: 无法删除文件
      VCTargetsPath\x64\Debug\VCTargetsPath.tlog\unsuccessfulbuild
    对路径
      D:\NKUcpp大作业\build_validation\CMakeFiles\4.3.2\VCTargetsPath\x64\Debug\VCTargetsPath.tlog\unsuccessfulbuild
    的访问被拒绝。
    ```

## Stopped Commands

- `cmake --build build --config Release`: not run because CMake configure did not complete.
- `.\build\Release\TinyRayStudio.exe --self-test`: not run because no validated build was produced.

## Result

- Validation is blocked before project compilation.
- The failure appears environmental: CMake/MSBuild cannot delete temporary files in generated build directories due to `拒绝访问` / access denied.
- No conclusion can be made yet about whether the Turntable code compiles or whether `--self-test` passes.

## Suggested Next Step

- Close any Visual Studio, MSBuild, CMake, antivirus scanner, or file indexer process that may be holding files in `build` or `build_validation`.
- Retry from a clean developer shell with write access to `D:\NKUcpp大作业`.
- If the directories remain locked, remove or rename `build` and `build_validation` after confirming no build process is using them, then rerun the validation commands.
