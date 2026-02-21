#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
生成 compile_commands.json 文件
用于 clangd 代码补全和静态分析
"""

import json
import os
from pathlib import Path

# 项目根目录
PROJECT_ROOT = Path(__file__).parent / "M600-D"

# 编译器和标志
COMPILER = "arm-none-eabi-gcc"
C_FLAGS = [
    "-std=c11",
    "-mcpu=cortex-m3",
    "-mthumb",
    "-Wall",
    "-fdata-sections",
    "-ffunction-sections",
    "-g",
    "-O0",
]

# 宏定义
DEFINES = [
    "-DSTM32F10X_HD",
    "-DUSE_STDPERIPH_DRIVER",
    "-D__CC_ARM",
]

# 头文件包含路径
INCLUDE_PATHS = [
    "User/APP",
    "User/DRV",
    "User/LIB",
    "User/SEGGER",
    "User/BackTrace",
    "User",
    "BSP",
    "Libraries/CMSIS",
    "Libraries/FWlib/inc",
]

# 源文件列表
SOURCE_FILES = [
    # User/APP
    "User/APP/app_system.c",
    "User/APP/app_comm.c",
    "User/APP/app_handcomm.c",
    "User/APP/app_memory.c",
    "User/APP/app_negprsheat.c",
    "User/APP/app_radiofreq.c",
    "User/APP/app_shockwave.c",
    "User/APP/app_treatmgr.c",
    "User/APP/app_ultrasound.c",
    # User/DRV
    "User/DRV/drv_24c02.c",
    "User/DRV/drv_adc.c",
    "User/DRV/drv_dac.c",
    "User/DRV/drv_delay.c",
    "User/DRV/drv_init.c",
    "User/DRV/drv_iodevice.c",
    "User/DRV/drv_memory.c",
    "User/DRV/drv_si5351.c",
    "User/DRV/drv_soft_i2c.c",
    "User/DRV/drv_tim.c",
    "User/DRV/drv_usart.c",
    "User/DRV/drv_wdg.c",
    # User/LIB
    "User/LIB/lib_aiic.c",
    "User/LIB/lib_ringbuffer.c",
    # User/SEGGER
    "User/SEGGER/log.c",
    "User/SEGGER/SEGGER_RTT.c",
    "User/SEGGER/SEGGER_RTT_printf.c",
    # User/BackTrace
    "User/BackTrace/cm_backtrace.c",
    # User
    "User/main.c",
    "User/stm32f103_it.c",
    "User/delay.c",
    "User/example.c",
    # BSP
    "BSP/bsp_24C02.c",
    "BSP/bsp_adc.c",
    "BSP/bsp_dac.c",
    "BSP/bsp_delay.c",
    "BSP/bsp_gpio.c",
    "BSP/bsp_i2c.c",
    "BSP/bsp_iwdg.c",
    "BSP/bsp_SI5351.c",
    "BSP/bsp_tim.c",
    "BSP/bsp_usart.c",
    # Libraries/CMSIS
    "Libraries/CMSIS/core_cm3.c",
    "Libraries/CMSIS/system_stm32f10x.c",
    # Libraries/FWlib/src
    "Libraries/FWlib/src/misc.c",
    "Libraries/FWlib/src/stm32f10x_adc.c",
    "Libraries/FWlib/src/stm32f10x_bkp.c",
    "Libraries/FWlib/src/stm32f10x_can.c",
    "Libraries/FWlib/src/stm32f10x_cec.c",
    "Libraries/FWlib/src/stm32f10x_crc.c",
    "Libraries/FWlib/src/stm32f10x_dac.c",
    "Libraries/FWlib/src/stm32f10x_dbgmcu.c",
    "Libraries/FWlib/src/stm32f10x_dma.c",
    "Libraries/FWlib/src/stm32f10x_exti.c",
    "Libraries/FWlib/src/stm32f10x_flash.c",
    "Libraries/FWlib/src/stm32f10x_fsmc.c",
    "Libraries/FWlib/src/stm32f10x_gpio.c",
    "Libraries/FWlib/src/stm32f10x_i2c.c",
    "Libraries/FWlib/src/stm32f10x_iwdg.c",
    "Libraries/FWlib/src/stm32f10x_pwr.c",
    "Libraries/FWlib/src/stm32f10x_rcc.c",
    "Libraries/FWlib/src/stm32f10x_rtc.c",
    "Libraries/FWlib/src/stm32f10x_sdio.c",
    "Libraries/FWlib/src/stm32f10x_spi.c",
    "Libraries/FWlib/src/stm32f10x_tim.c",
    "Libraries/FWlib/src/stm32f10x_usart.c",
    "Libraries/FWlib/src/stm32f10x_wwdg.c",
]


def generate_compile_commands():
    """生成 compile_commands.json"""
    compile_commands = []
    
    for source_file in SOURCE_FILES:
        # 构建完整路径
        file_path = PROJECT_ROOT / source_file
        if not file_path.exists():
            print(f"[WARNING] 文件不存在 {file_path}")
            continue
        
        # 构建编译命令
        command_parts = [COMPILER]
        command_parts.extend(C_FLAGS)
        command_parts.extend(DEFINES)
        
        # 添加包含路径
        for include_path in INCLUDE_PATHS:
            abs_include = (PROJECT_ROOT / include_path).resolve()
            command_parts.append(f"-I{abs_include}")
        
        # 添加源文件
        command_parts.extend(["-c", str(file_path.resolve())])
        
        # 创建编译命令条目
        entry = {
            "directory": str(PROJECT_ROOT.resolve()),
            "command": " ".join(command_parts),
            "file": str(file_path.resolve())
        }
        
        compile_commands.append(entry)
    
    # 写入 JSON 文件
    output_file = PROJECT_ROOT.parent / "compile_commands.json"
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(compile_commands, f, indent=2, ensure_ascii=False)
    
    print(f"[OK] 成功生成 compile_commands.json")
    print(f"  位置: {output_file}")
    print(f"  包含 {len(compile_commands)} 个源文件")


if __name__ == "__main__":
    generate_compile_commands()

