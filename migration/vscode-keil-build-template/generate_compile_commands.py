#!/usr/bin/env python3
"""Generate compile_commands.json for a Keil-based firmware project."""

import json
from pathlib import Path


WORKSPACE_ROOT = Path(__file__).parent
PROJECT_ROOT = WORKSPACE_ROOT / "<YOUR_PROJECT_DIR>"

COMPILER = "arm-none-eabi-gcc"
C_FLAGS = [
    "-std=c11",
    "-mcpu=<YOUR_CPU>",
    "-mthumb",
    "-Wall",
    "-fdata-sections",
    "-ffunction-sections",
    "-g",
    "-O0",
]

DEFINES = [
    "-DYOUR_DEFINES",
]

INCLUDE_PATHS = [
    "YOUR_INCLUDE_PATHS",
]

SOURCE_FILES = [
    "YOUR_SOURCE_FILES",
]


def generate_compile_commands() -> None:
    compile_commands = []

    for source_file in SOURCE_FILES:
        file_path = PROJECT_ROOT / source_file
        if not file_path.exists():
            print(f"[WARNING] Missing source file: {file_path}")
            continue

        command_parts = [COMPILER, *C_FLAGS, *DEFINES]

        for include_path in INCLUDE_PATHS:
            abs_include = (PROJECT_ROOT / include_path).resolve()
            command_parts.append(f"-I{abs_include}")

        command_parts.extend(["-c", str(file_path.resolve())])

        compile_commands.append(
            {
                "directory": str(PROJECT_ROOT.resolve()),
                "command": " ".join(command_parts),
                "file": str(file_path.resolve()),
            }
        )

    output_file = WORKSPACE_ROOT / "compile_commands.json"
    with output_file.open("w", encoding="utf-8") as file_handle:
        json.dump(compile_commands, file_handle, indent=2, ensure_ascii=False)

    print("[OK] Generated compile_commands.json")
    print(f"  Path: {output_file}")
    print(f"  Entries: {len(compile_commands)}")


if __name__ == "__main__":
    generate_compile_commands()

