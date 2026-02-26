# RF-M600 多功能医疗治疗设备

## 项目简介

RF-M600 是一款集成了四种治疗模式的医疗设备固件项目，基于 STM32F103CBT6 微控制器开发。

### 支持的治疗模式

- 🔊 **超声治疗 (Ultrasound)** - 1000-1400 kHz，40级功率调节
- 📡 **射频治疗 (Radio Frequency)** - 1MHz，20级功率调节
- ⚡ **冲击波治疗 (Shockwave)** - 26级强度，16级频率
- 🌡️ **负压热疗 (Negative Pressure Heat)** - 温控加热 + 真空循环

---

## 技术规格

| 项目 | 规格 |
|------|------|
| **MCU** | STM32F103CBT6 (ARM Cortex-M3) |
| **Flash** | 128KB |
| **RAM** | 20KB |
| **时钟** | 72MHz |
| **固件版本** | v1.0.0 |
| **硬件版本** | v1.0.0 |

---

## 项目结构

```
RF-M600/
├── M600-D/                          # 固件源码
│   ├── BSP/                         # 板级支持包
│   │   ├── bsp_gpio.c/h
│   │   ├── bsp_adc.c/h
│   │   ├── bsp_dac.c/h
│   │   ├── bsp_tim.c/h
│   │   ├── bsp_usart.c/h
│   │   ├── bsp_i2c.c/h
│   │   └── bsp_SI5351.c/h
│   ├── Libraries/                   # STM32标准库
│   │   ├── CMSIS/
│   │   └── FWlib/
│   ├── Project/                     # Keil工程文件
│   │   └── M600.uvprojx
│   └── User/                        # 用户代码
│       ├── APP/                     # 应用层
│       │   ├── app_system.c/h       # 系统管理
│       │   ├── app_comm.c/h         # 通信协议
│       │   ├── app_treatmgr.c/h     # 治疗管理器
│       │   ├── app_ultrasound.c/h   # 超声模块
│       │   ├── app_radiofreq.c/h    # 射频模块
│       │   ├── app_shockwave.c/h    # 冲击波模块
│       │   ├── app_negprsheat.c/h   # 热疗模块
│       │   ├── app_memory.c/h       # 参数存储
│       │   └── app_handcomm.c/h     # 手柄通信
│       ├── DRV/                     # 驱动层
│       │   ├── drv_init.c/h
│       │   ├── drv_usart.c/h
│       │   ├── drv_adc.c/h
│       │   ├── drv_dac.c/h
│       │   ├── drv_tim.c/h
│       │   ├── drv_si5351.c/h
│       │   ├── drv_24c02.c/h
│       │   ├── drv_iodevice.c/h
│       │   └── drv_wdg.c/h
│       ├── LIB/                     # 工具库
│       │   ├── lib_ringbuffer.c/h
│       │   └── lib_aiic.c/h
│       ├── SEGGER/                  # RTT日志
│       │   └── log.c/h
│       ├── BackTrace/               # 故障追踪
│       │   └── cm_backtrace.c/h
│       └── main.c
├── docs/                            # 文档
│   ├── architecture.md              # 架构设计
│   ├── protocol.md                  # 通信协议
│   ├── state_machine.md             # 状态机设计
│   └── safety.md                    # 安全机制
├── serial_assistant.py              # 串口调试工具
├── generate_compile_commands.py    # 编译命令生成
├── compile_commands.json            # clangd配置
├── requirements.txt                 # Python依赖
└── README.md                        # 本文件
```

---

## 快速开始

### 1. 环境准备

**硬件要求**:
- STM32F103CBT6 开发板
- J-Link 调试器
- USB转串口模块

**软件要求**:
- Keil MDK-ARM v5.x
- J-Link驱动
- Python 3.7+ (用于调试工具)

### 2. 编译固件

1. 打开 Keil 工程:
   ```
   M600-D/Project/M600.uvprojx
   ```

2. 选择目标配置并编译:
   ```
   Project → Build Target (F7)
   ```

3. 下载到设备:
   ```
   Flash → Download (F8)
   ```

### 3. 使用调试工具

安装 Python 依赖:
```bash
pip install -r requirements.txt
```

运行串口调试助手:
```bash
python serial_assistant.py
```

---

## 核心功能

### 系统架构

采用三层架构设计:
- **BSP层**: 硬件抽象，基于STM32标准库
- **DRV层**: 设备驱动，提供设备级API
- **APP层**: 应用逻辑，实现业务功能

详见: [架构设计文档](docs/architecture.md)

### 通信协议

基于串口的自定义二进制协议:
- 固定帧头: 0x5A 0xA5
- CRC16校验
- 支持4个治疗模块
- 支持3种命令类型

详见: [通信协议文档](docs/protocol.md)

### 状态机设计

多层状态机管理:
- 系统级状态机
- 治疗管理器状态机
- 各模块独立状态机
- 辅助状态机（PWM、真空控制等）

详见: [状态机设计文档](docs/state_machine.md)

### 安全机制

多重安全保护:
- 看门狗保护
- 探头连接检测
- 温度过热保护
- 电流过载保护
- 时间限制保护
- 参数范围检查

详见: [安全机制文档](docs/safety.md)

---

## 开发工具

### 1. 串口调试助手

**功能**:
- 串口通信测试
- 协议帧构建与解析
- 实时数据显示
- 支持所有治疗模块

**界面**:
- 左侧: 模块选择、命令选择、参数输入
- 右侧: 接收数据显示、发送数据显示

### 2. 日志系统

使用 SEGGER RTT 实时日志:
- 不占用UART资源
- 高速输出
- 支持多通道

**查看日志**:
1. 连接 J-Link
2. 打开 J-Link RTT Viewer
3. 选择目标设备
4. 查看实时日志

### 3. 故障追踪

集成 CmBacktrace:
- HardFault 自动分析
- 调用栈回溯
- 寄存器状态保存
- 故障原因定位

---

## 配置说明

### EEPROM 参数存储

使用 24C02 (256 Bytes) 存储:
- 治疗参数
- 剩余治疗次数
- 配置信息

**布局**:
```
0x00: Magic Number (0xA55A)
0x02: Version
0x04: Ultrasound Params (20B)
0x18: Radio Freq Params (16B)
0x28: Shockwave Params (20B)
0x3C: Heat Params (18B)
```

### 版本管理

版本号定义在 `app_system.h`:
```c
#define FW_VER_MAJOR  1
#define FW_VER_MINOR  0
#define FW_VER_PATCH  0
```

---

## 编译配置

### clangd 支持

生成 `compile_commands.json`:
```bash
python generate_compile_commands.py
```

用于:
- VSCode/Cursor 代码补全
- 语法检查
- 代码导航

### 编译选项

在 Keil 中配置:
- 优化级别: -O2
- 调试信息: 启用
- 警告级别: All Warnings

---

## 调试技巧

### 1. 查看实时状态

通过串口调试助手发送 "获取状态" 命令:
- 工作状态
- 温度
- 电流
- 剩余时间
- 错误码

### 2. 日志分析

日志级别:
- `LOG_E`: 错误 (红色)
- `LOG_W`: 警告 (黄色)
- `LOG_I`: 信息 (白色)
- `LOG_D`: 调试 (灰色)

### 3. 故障定位

HardFault 发生时:
1. 查看 RTT 日志中的 CmBacktrace 输出
2. 记录故障地址
3. 使用 addr2line 工具定位源码行

---

## 常见问题

### Q1: 编译错误 "missing header file"

**解决**: 检查 Keil 工程的 Include Paths 配置

### Q2: 下载失败

**解决**: 
1. 检查 J-Link 连接
2. 检查目标板供电
3. 尝试 "Erase Full Chip"

### Q3: 串口无数据

**解决**:
1. 检查波特率 (默认115200)
2. 检查串口号
3. 检查TX/RX接线

### Q4: 治疗无法启动

**解决**:
1. 检查探头是否连接
2. 检查脚踏开关状态
3. 查看错误码
4. 检查剩余治疗次数

---

## 贡献指南

### 代码规范

- 命名: `模块_功能_动作()` 格式
- 注释: 使用 Doxygen 风格
- 缩进: 4空格
- 编码: UTF-8

### 提交规范

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Type**:
- feat: 新功能
- fix: 修复
- docs: 文档
- style: 格式
- refactor: 重构
- test: 测试
- chore: 构建

---

## 版本历史

### v1.0.0 (2025-01)
- ✨ 初始版本
- ✅ 实现4种治疗模式
- ✅ 通信协议
- ✅ 安全保护机制
- ✅ 串口调试工具

---

## 许可证

Copyright (c) 2025 AstroCeta, Inc. All rights reserved.

---

## 联系方式

- **项目**: RF-M600
- **团队**: AstroCeta
- **文档**: [docs/](docs/)

---

## 相关文档

- [架构设计](docs/architecture.md)
- [通信协议](docs/protocol.md)
- [状态机设计](docs/state_machine.md)
- [安全机制](docs/safety.md)

---

**最后更新**: 2026-02-26
