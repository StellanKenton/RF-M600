# RF-M600 系统架构设计文档

## 1. 概述

RF-M600 是一款多功能医疗治疗设备，集成了四种治疗模式：
- **超声治疗 (Ultrasound)**
- **射频治疗 (Radio Frequency)**
- **冲击波治疗 (Shockwave)**
- **负压热疗 (Negative Pressure Heat)**

**硬件平台**: STM32F103CBT6 (ARM Cortex-M3)  
**固件版本**: v1.0.0  
**硬件版本**: v1.0.0

---

## 2. 系统分层架构

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer (APP)                  │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐  │
│  │  System  │  Comm    │ TreatMgr │  Memory  │ HandComm │  │
│  │  Manager │ Protocol │  Router  │  Config  │  Handle  │  │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘  │
│  ┌──────────┬──────────┬──────────┬──────────────────────┐ │
│  │UltraSound│RadioFreq │ShockWave │  NegPrsHeat          │ │
│  │  Module  │  Module  │  Module  │  Module              │ │
│  └──────────┴──────────┴──────────┴──────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                      Driver Layer (DRV)                      │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐  │
│  │  USART   │   ADC    │   DAC    │   TIM    │  I2C     │  │
│  ├──────────┼──────────┼──────────┼──────────┼──────────┤  │
│  │  WDG     │  Delay   │ IODevice │  Memory  │ SI5351   │  │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘  │
├─────────────────────────────────────────────────────────────┤
│                   Board Support Package (BSP)                │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐  │
│  │  GPIO    │   ADC    │   DAC    │   TIM    │  USART   │  │
│  ├──────────┼──────────┼──────────┼──────────┼──────────┤  │
│  │  I2C     │  IWDG    │  24C02   │  SI5351  │  Delay   │  │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘  │
├─────────────────────────────────────────────────────────────┤
│              STM32 Standard Peripheral Library               │
│                    (CMSIS + FWlib)                           │
└─────────────────────────────────────────────────────────────┘
```

### 2.1 分层职责

#### BSP层 (Board Support Package)
- 直接操作STM32外设寄存器
- 基于STM32标准外设库封装
- 提供硬件抽象接口
- 文件位置: `M600-D/BSP/`

#### DRV层 (Driver)
- 调用BSP层接口
- 实现设备驱动逻辑
- 提供设备级API
- 文件位置: `M600-D/User/DRV/`

#### APP层 (Application)
- 业务逻辑实现
- 治疗模块控制
- 通信协议处理
- 系统状态管理
- 文件位置: `M600-D/User/APP/`

---

## 3. 核心模块设计

### 3.1 系统管理模块 (app_system)

**职责**:
- 系统初始化
- 模式切换管理
- 主循环调度

**系统模式**:
```c
typedef enum {
    E_SYSTEM_STANDBY_MODE = 0,  // 待机模式
    E_SYSTEM_NORMAL_MODE,       // 正常工作模式
    E_SYSTEM_UPDATE_MODE,       // 固件升级模式
    E_SYSTEM_MODE_MAX
} System_Mode_EnumDef;
```

**初始化流程**:
```
System_Init()
├── Log_Init()                    // 日志系统初始化
├── Drv_WatchDogResartCheck()     // 看门狗重启检查
├── cm_backtrace_init()           // 故障追踪初始化
├── App_TreatMgr_Init()           // 治疗管理器初始化
├── App_Comm_Init()               // 通信协议初始化
└── App_HandComm_Init()           // 手柄通信初始化
```

**主循环**:
```c
void SystemProcess(void) {
    SystemManager();              // 系统状态机
    App_Memory_Process();         // 内存配置处理
    App_Comm_Process();           // 主机通信处理
    App_HandComm_Process();       // 手柄通信处理
    Log_Process(10);              // 日志处理
    Drv_WatchDogFeed();           // 喂狗
}
```

### 3.2 治疗管理器 (app_treatmgr)

**职责**:
- 探头检测与识别
- 脚踏开关检测
- 治疗模块路由
- 状态切换管理

**治疗状态**:
```c
typedef enum {
    E_TREATMGR_STATE_IDLE = 0,              // 空闲
    E_TREATMGR_STATE_RADIO_FREQUENCY,       // 射频治疗
    E_TREATMGR_STATE_SHOCK_WAVE,            // 冲击波治疗
    E_TREATMGR_STATE_NEGATIVE_PRESSURE_HEAT,// 负压热疗
    E_TREATMGR_STATE_ULTRASOUND,            // 超声治疗
    E_TREATMGR_STATE_ERROR,                 // 错误状态
    E_TREATMGR_STATE_MAX
} TreatMgr_State_EnumDef;
```

**探头检测**:
- 通过GPIO检测探头连接状态
- 防抖时间: 1000ms (100次 × 10ms)
- 根据探头类型自动切换治疗模式

**脚踏开关**:
- 检测脚踏开关状态 (开/关)
- 控制治疗启动/停止

### 3.3 通信协议模块 (app_comm)

**职责**:
- 与上位机通信
- 协议帧解析与封装
- 命令分发与响应
- 详见 [protocol.md](protocol.md)

**支持的模块**:
- 0x01: 超声模块
- 0x02: 射频模块
- 0x03: 冲击波模块
- 0x04: 热疗模块

**支持的命令**:
- 0x00: 获取状态
- 0x01: 设置工作状态
- 0x02: 设置配置参数

### 3.4 治疗模块

#### 3.4.1 超声治疗模块 (app_ultrasound)

**参数范围**:
- 频率: 1000-1400 kHz
- 电压: 1000-2000 (10-20V)
- 工作级别: 0-39 (40级)
- 温度限制: 350-480 (35-48℃)
- 工作时间: 最大3600秒

**运行状态**:
```c
typedef enum {
    E_US_RUN_INIT = 0,      // 初始化
    E_US_RUN_IDLE,          // 空闲
    E_US_RUN_WORKING,       // 工作中
    E_US_RUN_STOP,          // 停止
    E_US_RUN_WAIT_RETURN,   // 等待返回
    E_US_RUN_MAX
} US_RunState_EnumDef;
```

**错误检测**:
- 探头未连接
- 参数读取失败
- 参数无效
- 电流过高/过低
- 温度过高/过低
- 电压超限

#### 3.4.2 射频治疗模块 (app_radiofreq)

**参数范围**:
- 频率: 1000 kHz (固定)
- 电压: 11000-30000 mV (11-30V)
- 工作级别: 0-20
- 温度限制: 350-480 (35-48℃)
- 工作时间: 最大3600秒

**电压控制**:
- 初始电压: 7V
- 每级电压增量: (30V - 11V) / 20 = 0.95V
- 电流监测周期: 10ms
- 温度监测周期: 1000ms

#### 3.4.3 冲击波治疗模块 (app_shockwave)

**参数范围**:
- 工作级别: 1-26
- 频率级别: 1-16
- 最大工作点数: 10000
- 温度限制: 350-480 (35-48℃)

**PWM时序**:
- PWM_ESW+ 高电平时间: 5ms
- 等待时间: 17ms
- PWM_ESW-N 基础时间: 3ms
- PWM_ESW-N 步进: 0.28ms/级

**双路电流检测**:
- ESW-P 电流上下限
- ESW-N 电流上下限

#### 3.4.4 负压热疗模块 (app_negprsheat)

**参数范围**:
- 压力: 10-100 KPa
- 吸合时间: 100-60000 ms
- 释放时间: 100-60000 ms
- 温度限制: 350-480 (35-48℃)
- 预热温度限制: 350-480 (35-48℃)

**真空状态机**:
```c
typedef enum {
    E_NPH_VACUUM_STATE_IDLE = 0,      // 空闲
    E_NPH_VACUUM_STATE_SUCKING,       // 吸气中
    E_NPH_VACUUM_STATE_MAINTAIN,      // 保持
    E_NPH_VACUUM_STATE_RELEASING,     // 释放中
    E_NPH_VACUUM_STATE_MAX
} NPH_Vacuum_State_EnumDef;
```

**运行模式**:
- 预热模式: 仅加热，不启动真空
- 工作模式: 加热 + 真空循环

### 3.5 内存管理模块 (app_memory)

**职责**:
- EEPROM (24C02) 读写
- 治疗参数存储
- 配置参数管理

**存储内容**:
- 超声治疗参数
- 射频治疗参数
- 冲击波治疗参数
- 热疗治疗参数
- 剩余治疗次数

### 3.6 手柄通信模块 (app_handcomm)

**职责**:
- 与治疗手柄通信
- 手柄按键检测
- 手柄状态反馈

---

## 4. 关键驱动模块

### 4.1 IO设备驱动 (drv_iodevice)

**功能**:
- 探头连接检测
- 脚踏开关检测
- GPIO状态管理

### 4.2 ADC驱动 (drv_adc)

**功能**:
- 电流采样
- 温度采样 (NTC)
- 电压监测

### 4.3 DAC驱动 (drv_dac)

**功能**:
- 电压输出控制
- 功率调节

### 4.4 定时器驱动 (drv_tim)

**功能**:
- PWM输出 (超声/冲击波)
- 时间基准
- 周期控制

### 4.5 SI5351驱动 (drv_si5351)

**功能**:
- 时钟发生器
- 频率合成
- 用于超声频率生成

### 4.6 EEPROM驱动 (drv_24c02)

**功能**:
- I2C通信
- 参数存储
- 数据持久化

### 4.7 看门狗驱动 (drv_wdg)

**功能**:
- 系统复位检测
- 定时喂狗
- 故障恢复

---

## 5. 辅助模块

### 5.1 日志系统 (SEGGER RTT)

**功能**:
- 实时日志输出
- 无需UART占用
- 支持J-Link调试

**日志级别**:
- LOG_E: 错误
- LOG_W: 警告
- LOG_I: 信息
- LOG_D: 调试

### 5.2 故障追踪 (CmBacktrace)

**功能**:
- HardFault分析
- 调用栈回溯
- 寄存器状态保存
- 故障原因定位

### 5.3 环形缓冲区 (lib_ringbuffer)

**功能**:
- UART接收缓冲
- 数据队列管理

---

## 6. 数据流

### 6.1 命令处理流程

```
上位机 → UART → App_Comm_Process()
                    ↓
              协议解析 (CRC校验)
                    ↓
              命令分发 (按模块)
                    ↓
    ┌───────────────┼───────────────┐
    ↓               ↓               ↓
超声模块        射频模块        冲击波模块
    ↓               ↓               ↓
  执行命令        执行命令        执行命令
    ↓               ↓               ↓
  构建响应        构建响应        构建响应
    └───────────────┼───────────────┘
                    ↓
              App_Comm_Process()
                    ↓
              UART → 上位机
```

### 6.2 治疗控制流程

```
探头连接检测 → TreatMgr识别探头类型
                    ↓
              切换到对应治疗模块
                    ↓
              等待上位机命令
                    ↓
              设置工作参数
                    ↓
              脚踏开关按下
                    ↓
              启动治疗
                    ↓
    ┌───────────────┼───────────────┐
    ↓               ↓               ↓
  ADC采样      温度监测      时间计数
    ↓               ↓               ↓
  电流检测      过温保护      倒计时
    ↓               ↓               ↓
  过流保护      NTC检测      时间到停止
    └───────────────┼───────────────┘
                    ↓
              脚踏开关释放 / 时间到
                    ↓
              停止治疗
                    ↓
              上报状态
```

---

## 7. 任务调度

### 7.1 主循环轮询

```c
while(1) {
    SystemManager();           // 系统状态机
    App_TreatMgr_Process();    // 治疗管理 (10ms周期)
    App_Memory_Process();      // 内存处理
    App_Comm_Process();        // 通信处理 (5ms周期)
    App_HandComm_Process();    // 手柄通信
    Log_Process(10);           // 日志处理
    Drv_WatchDogFeed();        // 喂狗
}
```

### 7.2 任务周期

| 任务 | 周期 | 优先级 | 说明 |
|------|------|--------|------|
| TreatMgr_Process | 10ms | 高 | 探头检测、状态切换 |
| Comm_Process | 5ms | 中 | 通信协议处理 |
| Memory_Process | 按需 | 低 | 参数读写 |
| Log_Process | 10ms | 低 | 日志输出 |
| WatchDog_Feed | 每循环 | 最高 | 看门狗喂狗 |

---

## 8. 内存布局

### 8.1 Flash分区

```
0x08000000 ┌─────────────────┐
           │  Bootloader     │ (预留)
0x08004000 ├─────────────────┤
           │  Application    │
           │                 │
           │                 │
0x0801FFFF └─────────────────┘
```

### 8.2 RAM使用

```
0x20000000 ┌─────────────────┐
           │  Stack          │
           ├─────────────────┤
           │  Heap           │
           ├─────────────────┤
           │  Global Data    │
           │  - System Mgr   │
           │  - Treat Mgr    │
           │  - Comm Buffers │
           │  - Module Data  │
0x20004FFF └─────────────────┘
```

### 8.3 EEPROM布局 (24C02, 256 Bytes)

```
0x00 ┌─────────────────────┐
     │  Magic Number (2B)  │
0x02 ├─────────────────────┤
     │  Version (2B)       │
0x04 ├─────────────────────┤
     │  US Params (20B)    │
0x18 ├─────────────────────┤
     │  RF Params (16B)    │
0x28 ├─────────────────────┤
     │  SW Params (20B)    │
0x3C ├─────────────────────┤
     │  NPH Params (18B)   │
0x4E ├─────────────────────┤
     │  Reserved           │
0xFF └─────────────────────┘
```

---

## 9. 编译与构建

### 9.1 工具链

- **IDE**: Keil MDK-ARM
- **编译器**: ARMCC
- **调试器**: J-Link
- **项目文件**: `M600-D/Project/M600.uvprojx`

### 9.2 编译命令生成

使用Python脚本生成 `compile_commands.json`:
```bash
python generate_compile_commands.py
```

用于支持：
- clangd语言服务器
- VSCode/Cursor代码补全
- 静态分析工具

---

## 10. 调试工具

### 10.1 串口调试助手

**文件**: `serial_assistant.py`

**功能**:
- 串口通信测试
- 协议帧构建
- 数据解析显示
- 支持所有4个治疗模块

**使用**:
```bash
pip install -r requirements.txt
python serial_assistant.py
```

### 10.2 日志查看

使用 J-Link RTT Viewer:
1. 连接J-Link
2. 打开RTT Viewer
3. 选择目标设备
4. 查看实时日志

---

## 11. 版本管理

### 11.1 版本号定义

```c
#define FW_VER_MAJOR  1
#define FW_VER_MINOR  0
#define FW_VER_PATCH  0
#define FIRMWARE_VERSION "v1.0.0"

#define HW_VER_MAJOR  1
#define HW_VER_MINOR  0
#define HW_VER_PATCH  0
#define HARDWARE_VERSION "v1.0.0"
```

### 11.2 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| v1.0.0 | 2025-01 | 初始版本 |

---

## 12. 未来改进方向

### 12.1 短期优化
- [ ] 增加命令超时和重传机制
- [ ] 统一错误处理框架
- [ ] 完善参数版本管理
- [ ] 增强安全保护机制

### 12.2 中期优化
- [ ] 引入RTOS (FreeRTOS)
- [ ] 实现OTA固件升级
- [ ] 增加数据记录功能
- [ ] 优化功率控制算法

### 12.3 长期规划
- [ ] 支持更多治疗模式
- [ ] 增加网络连接功能
- [ ] 云端数据同步
- [ ] AI辅助治疗参数优化

---

## 13. 参考文档

- [通信协议设计](protocol.md)
- [状态机设计](state_machine.md)
- [安全机制说明](safety.md)
- STM32F103 Reference Manual
- CmBacktrace Documentation
- SEGGER RTT Documentation

---

**文档版本**: v1.0  
**最后更新**: 2026-02-26  
**维护者**: AstroCeta Team

