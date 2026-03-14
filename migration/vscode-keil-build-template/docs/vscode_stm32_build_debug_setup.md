# VS Code STM32 编译模板说明

这是一份用于迁移的模板文档，只保留 VS Code 调用 Keil 构建并把结果回显到终端的能力。

这份模板不绑定任何具体项目，复制到新工程后，先替换占位符，再执行构建。

## 1. 模板包含的文件

- `.vscode/tasks.json`
- `.vscode/scripts/keil-build.ps1`
- `.vscode/extensions.json`
- `generate_compile_commands.py`

各文件职责：

- `tasks.json`: 定义 VS Code 中的 Build 和 Rebuild 任务
- `keil-build.ps1`: 调用 `UV4.exe`，轮询 `build_log.txt`，并把结果打印到终端
- `extensions.json`: 推荐基础 C/C++ 扩展
- `generate_compile_commands.py`: 为编辑器生成 `compile_commands.json`

## 2. 需要替换的占位符

在开始使用前，至少要替换：

- `<YOUR_PROJECT_DIR>`: 工程目录名
- `<YOUR_PROJECT_NAME>`: `uvprojx` 文件名，不带扩展名时也要保持和文件一致
- `<YOUR_TARGET_NAME>`: Keil 里的 target 名称
- `<YOUR_CPU>`: 例如 `cortex-m3`、`cortex-m4`
- `YOUR_DEFINES`: 工程宏定义
- `YOUR_INCLUDE_PATHS`: 头文件目录列表
- `YOUR_SOURCE_FILES`: 源文件列表

## 3. 模板使用步骤

### 3.1 安装依赖软件

需要安装：

- Keil MDK-ARM v5.x
- VS Code

建议安装扩展：

- `ms-vscode.cpptools`

### 3.2 确认 `UV4.exe`

在 PowerShell 中执行：

```powershell
Get-Command UV4.exe
```

如果没有结果，可以手动设置：

```powershell
setx KEIL_UV4 "C:\Path\To\UV4.exe"
```

设置后重启 VS Code。

### 3.3 修改 `.vscode/tasks.json`

需要替换：

- `-ProjectPath`
- `-Target`
- `problemMatchers[].fileLocation`

示例：

```json
"-ProjectPath",
"${workspaceFolder}/App/Project/AppBoard.uvprojx",
"-Target",
"Release"
```

### 3.4 修改 `generate_compile_commands.py`

需要替换：

- `PROJECT_ROOT`
- `C_FLAGS`
- `DEFINES`
- `INCLUDE_PATHS`
- `SOURCE_FILES`

生成方式：

```powershell
py -3 generate_compile_commands.py
```

## 4. 构建执行链路

在 VS Code 中按 `Ctrl+Shift+B`，执行 `Keil: Build`。

执行过程如下：

1. VS Code 调用 `.vscode/tasks.json` 中的构建任务
2. 任务执行 `.vscode/scripts/keil-build.ps1`
3. 脚本查找 `UV4.exe`
4. 脚本执行 `UV4.exe -b <project> -t <target> -o build_log.txt`
5. 脚本轮询 `build_log.txt`
6. 脚本把最终结果回显到终端
7. 如果构建失败，任务返回非 0 退出码

## 5. 常见问题

### 5.1 编译器版本不匹配

常见报错：

- `Target uses ARM-Compiler 'Vx.xx' which is not available`

处理方式：

- 打开对应的 `uvprojx` 检查编译器配置
- 或者在 Keil 中重新选择可用编译器版本并保存工程

### 5.2 终端里只有启动信息，没有最终结果

常见现象：

- 终端里只有 `Using Keil`、`Project`、`Target`、`Mode`

处理方式：

- 保持 `keil-build.ps1` 的日志轮询方式
- 不要直接把 VS Code 任务改成调用 `UV4.exe`

## 6. 给 AI 的迁移说明模板

```text
请把这个 VS Code Keil 编译模板迁移到新项目。

需要保留的能力：
1. Ctrl+Shift+B 通过 Keil 命令行构建
2. 编译结果在 VS Code 终端里可见
3. 构建失败时返回非 0 退出码

请重点参考这些文件：
- .vscode/tasks.json
- .vscode/scripts/keil-build.ps1
- generate_compile_commands.py

请替换这些占位符：
- <YOUR_PROJECT_DIR>
- <YOUR_PROJECT_NAME>
- <YOUR_TARGET_NAME>
- <YOUR_CPU>
- YOUR_DEFINES
- YOUR_INCLUDE_PATHS
- YOUR_SOURCE_FILES
```