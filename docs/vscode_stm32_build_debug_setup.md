# VS Code STM32 编译与调试环境迁移说明

这份文档用于两类场景：

- 把当前项目在 VS Code 下的编译和调试能力移植到其他 STM32 或 GD32 固件项目
- 在另一台 Windows 电脑上快速重建同一套开发环境

当前仓库已经验证通过的能力：

- `Ctrl+Shift+B` 触发 Keil 命令行编译
- `F5` 通过 Cortex-Debug + J-Link 进行下载和调试
- 支持 SEGGER RTT 控制台
- 支持 Cortex-Debug Live Watch 监看全局/静态变量

## 1. 当前项目依赖的关键文件

当前方案主要由下面几个文件组成：

- `.vscode/launch.json`
- `.vscode/tasks.json`
- `.vscode/settings.json`
- `.vscode/extensions.json`
- `.vscode/scripts/keil-build.ps1`
- `generate_compile_commands.py`
- `compile_commands.json`

各文件职责：

- `launch.json`: 配置 Cortex-Debug、J-Link、GDB、RTT、Live Watch
- `tasks.json`: 配置 VS Code 构建任务，调用 PowerShell 脚本执行 Keil 构建
- `settings.json`: 配置 C/C++ 补全和 Cortex-Debug Live Watch 刷新率
- `extensions.json`: 推荐安装必需的 VS Code 扩展
- `keil-build.ps1`: 真正执行 `UV4.exe`，等待 `build_log.txt` 完成并把结果回显到终端
- `generate_compile_commands.py`: 为 clangd / C/C++ 生成 `compile_commands.json`

## 2. 当前项目的实际配置参数

这份项目当前使用的是：

- IDE: Keil MDK-ARM 5
- 编译器: ARM Compiler 5.06 update 7
- 调试器: SEGGER J-Link
- GDB Server: `JLinkGDBServerCL.exe`
- GDB: GNU Arm Toolchain 自带 `arm-none-eabi-gdb.exe`
- 调试扩展: `marus25.cortex-debug`

当前工程里的关键值：

- Keil 工程文件: `M600-D/Project/M600.uvprojx`
- 构建目标名: `Development`
- 输出 ELF/AXF: `M600-D/Project/Objects/M600.axf`
- 芯片名: `GD32F103RC`
- 调试接口: `SWD`

注意：

- README 里曾写过 `STM32F103CBT6`，但 Keil 工程里实际配置的是 `GD32F103RC`
- 调试和下载时应该以 `M600.uvprojx` 中的真实设备型号为准

## 3. 另一台电脑上如何重建这套环境

### 3.1 必装软件

需要安装：

- Keil MDK-ARM v5.x
- J-Link Software and Documentation Pack
- GNU Arm Toolchain for Windows
- VS Code

VS Code 里需要安装扩展：
- `ms-vscode.cpptools`

### 3.2 需要确认的可执行文件
- `UV4.exe`
- `JLinkGDBServerCL.exe`
- `arm-none-eabi-gdb.exe`

建议确认方式：
Get-Command UV4.exe, JLinkGDBServerCL.exe, arm-none-eabi-gdb.exe
```

```powershell
setx KEIL_UV4 "C:\Path\To\UV4.exe"
```

然后重启 VS Code。

Windows 下 `Program Files (x86)` 这类路径有时会让某些调试扩展表现不稳定。当前项目已验证可用的做法是：


## 4. 当前工作区里已经实现的能力

### 4.1 编译

在 VS Code 中按 `Ctrl+Shift+B` 执行 `Keil: Build`。

1. 调用 `.vscode/scripts/keil-build.ps1`
2. 脚本定位 `UV4.exe`
3. 执行 `UV4.exe -b <project> -t <target> -o build_log.txt`
4. 轮询 `build_log.txt`
5. 等待 Keil 写出最终结果
7. 如果出现错误，任务返回非 0 退出码

### 4.2 调试

在 VS Code 中按 `F5`，使用 `STM32: J-Link Launch`。

调试链路是：

1. 先执行 `Keil: Build`
2. 启动 `JLinkGDBServerCL.exe`
3. 启动 `arm-none-eabi-gdb.exe`
4. 连接 `M600.axf`
5. 进入 `main`

### 4.3 RTT

当前 `launch.json` 已启用 `rttConfig`。

只要固件里有 SEGGER RTT 并成功初始化，调试启动后会出现 RTT 控制台。

### 4.4 Live Watch

当前 `launch.json` 已启用：

```json
"liveWatch": {
  "enabled": true,
  "samplesPerSecond": 4
}
```

`settings.json` 里还设置了：

```json
"cortex-debug.liveWatchRefreshRate": 250
```


### 5.1 修改 `tasks.json`

需要改：

- `-ProjectPath`
- `-Target`

例如：
"-ProjectPath",
"${workspaceFolder}/Foo/Project/Foo.uvprojx",
"-Target",
"Debug"
```

### 5.2 修改 `launch.json`

需要改：

- `device`
- `executable`
- `interface`，通常是 `swd`
- `preLaunchTask`，如果构建任务名变了也要同步改

```json
"device": "STM32F103C8",
"executable": "${workspaceFolder}/Project/Objects/Foo.axf"
```

### 5.3 修改 `keil-build.ps1`
通常不用改脚本逻辑，只需要：

- 保证它能找到 `UV4.exe`

### 5.4 修改 `generate_compile_commands.py`

- `PROJECT_ROOT`
- `DEFINES`
- `INCLUDE_PATHS`
- `SOURCE_FILES`


```powershell
py -3 generate_compile_commands.py

## 6. 换电脑后最常见的问题

症状：

- `Target uses ARM-Compiler 'Vx.xx' which is not available`

原因：

- `M600.uvprojx` 里写死了某个 AC5/AC6 版本，但新电脑装的是另一个版本

处理方式：

- 打开 `uvprojx` 检查 `pArmCC` 和 `pCCUsed`
- 或者在 Keil 里重新选择可用编译器版本并保存工程

### 6.2 VS Code 里只看到 Keil 启动信息，看不到编译结果

- 终端里只有 `Using Keil`, `Project`, `Target`, `Mode`

- 直接调用 GUI 版 `UV4.exe` 时，PowerShell 可能被常驻进程挂住

处理方式：

- 使用当前仓库里的 `keil-build.ps1`
- 脚本已改成异步启动并轮询 `build_log.txt`

症状：

- `Unable to start GDB even after 5 seconds`
- `Could not start gdb, no response from gdb`

处理方式：

- 先在命令行单独运行 GDB
- 在 `launch.json` 显式设置 `armToolchainPath`
- 显式设置 `gdbPath` 和 `objdumpPath`
- 必要时改用 DOS 短路径
- 给 `debuggerArgs` 加 `--nx`

### 6.4 普通 Watch 不自动刷新

这是正常行为。

- VS Code 的普通 `Watch` / `Variables` 是停机态观察模型
- 程序运行时不会持续刷新
- 想在程序运行时看值，要用 `Live Watch`

## 7. Live Watch 如何使用

`Live Watch` 和普通 `Watch` 不是一回事。

区别如下：

- 普通 `Watch`: 适合断点停住后查看局部变量、表达式、栈帧变量
- `Live Watch`: 适合程序运行中持续观察全局变量和静态变量

### 7.1 使用前提

当前工程已经打开 `liveWatch`，所以只需要：

1. 启动调试
2. 确保 J-Link 已正常连接
3. 程序已经在运行状态

### 7.2 打开方式

在 VS Code 中：

1. 打开命令面板
2. 搜索 `Cortex-Debug: View Live Watch`
3. 打开 `Live Watch` 面板

如果侧边栏里已经有 Cortex-Debug 的树视图，也可以直接在那里看到。

### 7.3 添加变量

推荐添加这类变量：

- 全局变量
- 文件内 `static` 变量
- 全局结构体成员，例如 `g_system.state`
- 外设状态缓存变量，例如 `adc_raw[0]`

不推荐添加：

- 普通局部变量
- 只在某个函数栈帧内存在的自动变量

添加方式：

1. 在 `Live Watch` 面板里点击 `+`
2. 输入表达式，例如：

```c
g_system_state
adc_value
adc_buffer[0]
app_ctx.mode
```

### 7.4 从变量窗口直接添加

如果某个变量在暂停状态下能在 Variables 窗口看到，也可以尝试通过 Cortex-Debug 提供的命令直接加到 Live Watch。

但要注意：

- 只有全局/静态变量适合放进 Live Watch
- 局部变量即使能临时加进去，运行起来后通常也会失效或不更新

### 7.5 刷新行为

当前配置下：

- `samplesPerSecond = 4`
- `liveWatchRefreshRate = 250 ms`

也就是大约每秒刷新 4 次。

如果你觉得太快或太慢，可以调整：

- `launch.json` 里的 `liveWatch.samplesPerSecond`
- `settings.json` 里的 `cortex-debug.liveWatchRefreshRate`

建议范围：

- 低频状态量: `2` 到 `4`
- 变化较快但又不想影响调试稳定性: `5` 到 `10`

不要一开始就设太高，J-Link 和目标机会承受额外读取负载。

### 7.6 Live Watch 不更新时怎么判断

优先检查：

1. 变量是不是全局或静态变量
2. 变量名是不是表达式写错了
3. 变量是否被优化掉
4. 当前调试会话是否就是启用了 `liveWatch` 的那一个配置

如果变量始终不刷新，可以先把程序暂停，确认这个变量在暂停时是否可读。如果暂停时都不可读，说明不是 Live Watch 的问题，而是符号、作用域或优化问题。

## 8. 给 AI 迁移这套方案时可以直接提供的信息

如果后面你想让 AI 把这套方案迁移到别的项目，可以直接把下面这份信息发给它：

```text
请把当前仓库里的 VS Code STM32 编译和调试方案迁移到新项目。

需要保留的能力：
1. Ctrl+Shift+B 通过 Keil 命令行构建
2. F5 通过 Cortex-Debug + J-Link 调试
3. 支持 RTT
4. 支持 Live Watch

请重点参考这些文件：
- .vscode/launch.json
- .vscode/tasks.json
- .vscode/settings.json
- .vscode/scripts/keil-build.ps1

迁移时请替换：
- Keil 工程路径
- Target 名称
- 芯片 device 名称
- 输出 axf 路径
- 如果项目源码结构不同，还要同步调整 generate_compile_commands.py

如果新电脑上的 Keil 或 GNU Arm Toolchain 路径不同，请优先做成可配置，而不是写死绝对路径。
```

## 9. 建议的复用策略

如果后面会经常迁移，建议把下面这些东西作为模板保留：

- `.vscode/launch.json`
- `.vscode/tasks.json`
- `.vscode/settings.json`
- `.vscode/extensions.json`
- `.vscode/scripts/keil-build.ps1`

然后在新项目中只替换 4 个地方：

1. Keil 工程路径
2. Target 名称
3. 芯片型号
4. AXF 输出路径

这样迁移成本最低。