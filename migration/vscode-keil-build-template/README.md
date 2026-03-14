# VS Code Keil Build Template

这个目录是一个“仅编译”模板包，用来给 STM32/GD32 固件项目复用 VS Code + Keil 构建能力。

这个模板故意不保留具体项目名、Target 名称和生成产物，复制后需要先替换占位符，再执行构建。

模板目录结构：

- `.vscode/tasks.json`
- `.vscode/extensions.json`
- `.vscode/scripts/keil-build.ps1`
- `generate_compile_commands.py`
- `docs/vscode_stm32_build_debug_setup.md`

首次使用前至少要替换这些占位符：

1. `<YOUR_PROJECT_DIR>`
2. `<YOUR_PROJECT_NAME>`
3. `<YOUR_TARGET_NAME>`
4. `YOUR_DEFINES`
5. `YOUR_INCLUDE_PATHS`
6. `YOUR_SOURCE_FILES`

建议替换顺序：

1. 先改 `.vscode/tasks.json`
2. 再改 `generate_compile_commands.py`
3. 最后运行 `py -3 generate_compile_commands.py`

说明：

- `keil-build.ps1` 可以直接复用，一般不需要改
- `compile_commands.json` 不再随模板分发，应该在新项目中重新生成
- 如果 `UV4.exe` 不在系统 PATH 中，优先设置环境变量 `KEIL_UV4`