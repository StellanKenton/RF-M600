# VS Code STM32 编译环境迁移说明

这份文档只保留一件事：如何在 VS Code 中调用 Keil 命令行编译，并把编译结果稳定回显到终端，方便迁移到另一台 Windows 电脑或另一个 STM32/GD32 固件项目。

当前仓库已经验证通过的能力：

- `Ctrl+Shift+B` 触发 Keil 命令行编译
- 编译日志最终会返回到 VS Code 终端
- 编译失败时任务返回非 0 退出码，便于直接发现错误

## 1. 当前方案依赖的关键文件

当前方案主要由下面几个文件组成：

- `.vscode/tasks.json`
- `.vscode/scripts/keil-build.ps1`
- `.vscode/extensions.json`
- `generate_compile_commands.py`
- `compile_commands.json`

各文件职责：

- `tasks.json`: 配置 VS Code 构建任务，触发 PowerShell 脚本执行 Keil 构建
- `keil-build.ps1`: 定位 `UV4.exe`，执行构建，并把 `build_log.txt` 中的结果回显到终端
- `extensions.json`: 推荐安装基础 C/C++ 扩展
- `generate_compile_commands.py`: 为 clangd / C/C++ 生成 `compile_commands.json`
- `compile_commands.json`: 给编辑器提供头文件路径、宏和源码索引，不参与实际构建

## 2. 当前项目的实际配置参数

这份项目当前使用的是：

- IDE: Keil MDK-ARM 5
- 编译器: ARM Compiler 5.06 update 7

当前工程里的关键值：

- Keil 工程文件: `M600-D/Project/M600.uvprojx`
- 构建目标名: `Development`
- 输出文件: `M600-D/Project/Objects/M600.axf`

## 3. 另一台电脑上如何重建这套环境

### 3.1 必装软件

需要安装：

- Keil MDK-ARM v5.x
- VS Code

VS Code 里建议安装扩展：

- `ms-vscode.cpptools`

### 3.2 需要确认的可执行文件

至少需要确认：

- `UV4.exe`

建议在 PowerShell 中执行：

```powershell
Get-Command UV4.exe
```

如果系统环境变量里找不到 `UV4.exe`，可以手动设置：

```powershell
setx KEIL_UV4 "C:\Path\To\UV4.exe"
```

设置后重启 VS Code。

## 4. 当前工作区里已经实现的编译能力

在 VS Code 中按 `Ctrl+Shift+B`，执行 `Keil: Build`。

执行链路如下：

1. VS Code 调用 `.vscode/tasks.json` 里的 `Keil: Build`
2. 任务执行 `.vscode/scripts/keil-build.ps1`
3. 脚本定位 `UV4.exe`
4. 脚本执行 `UV4.exe -b <project> -t <target> -o build_log.txt`
5. 脚本轮询 `build_log.txt`
6. Keil 写完结果后，脚本把关键输出回显到终端
7. 如果构建失败，任务返回非 0 退出码

这样做的目的不是直接依赖 `UV4.exe` 的控制台输出，而是通过日志文件获取最终编译结果，避免 VS Code 终端里只看到启动信息却看不到编译完成状态。

## 5. 迁移到别的项目时需要改什么

### 5.1 修改 `tasks.json`

主要改这两个参数：

- `-ProjectPath`
- `-Target`

示例：

```json
"-ProjectPath",
"${workspaceFolder}/Foo/Project/Foo.uvprojx",
"-Target",
"Release"
```

### 5.2 保留 `keil-build.ps1`

通常不需要改脚本逻辑，只需要确保：

- 脚本能找到 `UV4.exe`
- 输出日志路径仍然有效
- 新项目的 `uvprojx` 和 target 名称正确

如果只是换电脑，优先保持脚本不动，只修正环境变量或可执行文件路径。

### 5.3 按需更新 `generate_compile_commands.py`

如果你希望新项目在 VS Code 里也保留较完整的补全、跳转和头文件索引，需要同步调整：

- `PROJECT_ROOT`
- `DEFINES`
- `INCLUDE_PATHS`
- `SOURCE_FILES`

生成方式：

```powershell
py -3 generate_compile_commands.py
```

这一步只影响编辑器体验，不影响 Keil 实际编译。

## 6. 换电脑后最常见的问题

### 6.1 编译器版本不匹配

常见报错：

- `Target uses ARM-Compiler 'Vx.xx' which is not available`

原因：

- `uvprojx` 中固定了某个 AC5/AC6 版本，但新电脑安装的是另一个版本

处理方式：

- 打开 `M600.uvprojx` 检查 `pArmCC` 和 `pCCUsed`
- 或者在 Keil 中重新选择可用编译器版本并保存工程

### 6.2 VS Code 终端里只看到启动信息，看不到编译结果

常见现象：

- 终端里只有 `Using Keil`、`Project`、`Target`、`Mode`
- 但没有最终的 `Error(s)`、`Warning(s)` 或成功结束信息

原因：

- 直接调用 GUI 版 `UV4.exe` 时，PowerShell 不一定能拿到完整控制台输出

处理方式：

- 使用当前仓库里的 `keil-build.ps1`
- 保持脚本里的异步启动和日志轮询逻辑
- 不要把构建任务改回成直接调用 `UV4.exe`

## 7. 建议保留的最小模板

如果后面需要迁移到其他电脑或其他固件项目，建议至少保留：

- `.vscode/tasks.json`
- `.vscode/scripts/keil-build.ps1`
- `.vscode/extensions.json`

如果还希望补全和跳转体验一致，再额外保留：

- `generate_compile_commands.py`
- `compile_commands.json`

## 8. 给 AI 迁移这套方案时可以直接提供的信息

如果后面想让 AI 帮你迁移到新项目，可以直接给出下面这段说明：

```text
请把当前仓库里的 VS Code Keil 编译方案迁移到新项目。

需要保留的能力：
1. Ctrl+Shift+B 通过 Keil 命令行构建
2. 编译结果在 VS Code 终端里可见
3. 构建失败时返回非 0 退出码

请重点参考这些文件：
- .vscode/tasks.json
- .vscode/scripts/keil-build.ps1
- generate_compile_commands.py

迁移时请替换：
- Keil 工程路径
- Target 名称
- 输出 axf 路径
- 如果项目源码结构不同，再同步调整 generate_compile_commands.py

如果新电脑上的 Keil 路径不同，请优先做成可配置，而不是写死绝对路径。
```