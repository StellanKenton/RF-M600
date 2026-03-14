# VS Code Keil Build Bundle

这个目录是从当前仓库提取出来的“仅编译”迁移包。

用途：

- 在另一台 Windows 电脑上复用当前项目的 VS Code + Keil 构建能力
- 作为其他 STM32/GD32 项目的迁移模板

建议保留的目录结构：

- `.vscode/tasks.json`
- `.vscode/extensions.json`
- `.vscode/scripts/keil-build.ps1`
- `generate_compile_commands.py`
- `compile_commands.json`
- `docs/vscode_stm32_build_debug_setup.md`

直接复用到另一台电脑时，优先检查：

1. `UV4.exe` 是否可通过环境变量 `KEIL_UV4` 或系统 PATH 找到
2. Keil 工程路径是否仍是 `M600-D/Project/M600.uvprojx`
3. Target 名称是否仍是 `Development`

如果迁移到另一个项目，需要至少修改：

1. `.vscode/tasks.json` 里的 `-ProjectPath`
2. `.vscode/tasks.json` 里的 `-Target`
3. `.vscode/tasks.json` 里的 `problemMatchers[].fileLocation`
4. `generate_compile_commands.py` 里的工程根目录、头文件路径、宏和源文件列表

如果只想在另一台电脑上复用当前项目，通常不需要改 `keil-build.ps1`。