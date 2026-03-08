# RF-M600 状态机设计文档

## 1. 概述

RF-M600 系统采用多层状态机设计，包括：
- 系统级状态机
- 治疗管理器状态机
- 各治疗模块状态机
- 辅助状态机（真空控制、PWM控制等）

---

## 2. 系统级状态机

### 2.1 状态定义

```c
typedef enum {
    E_SYSTEM_STANDBY_MODE = 0,  // 待机模式
    E_SYSTEM_NORMAL_MODE,       // 正常工作模式
    E_SYSTEM_UPDATE_MODE,       // 固件升级模式
    E_SYSTEM_MODE_MAX
} System_Mode_EnumDef;
```

### 2.2 状态转换图

```
        ┌─────────────────┐
        │   Power On      │
        └────────┬────────┘
                 │
                 ▼
        ┌─────────────────┐
        │  STANDBY_MODE   │◄──────────┐
        └────────┬────────┘           │
                 │                     │
                 │ 自动切换             │ 错误恢复
                 ▼                     │
        ┌─────────────────┐           │
        │  NORMAL_MODE    │───────────┘
        └────────┬────────┘
                 │
                 │ 升级命令
                 ▼
        ┌─────────────────┐
        │  UPDATE_MODE    │
        └─────────────────┘
```

### 2.3 状态说明

| 状态 | 说明 | 进入条件 | 退出条件 |
|------|------|----------|----------|
| STANDBY_MODE | 待机模式，系统初始化完成 | 上电复位 | 自动切换 |
| NORMAL_MODE | 正常工作模式 | 初始化完成 | 升级命令 |
| UPDATE_MODE | 固件升级模式 | 收到升级命令 | 升级完成/失败 |

### 2.4 状态处理

```c
void SystemManager(void)
{
    switch(s_SystemMgr.eMode)
    {
        case E_SYSTEM_STANDBY_MODE:
            // 待机模式处理
            System_ChangeMode(E_SYSTEM_NORMAL_MODE);
            break;
            
        case E_SYSTEM_NORMAL_MODE:
            // 正常模式处理
            App_TreatMgr_Process();  // 治疗管理
            break;
            
        case E_SYSTEM_UPDATE_MODE:
            // 升级模式处理
            // TODO: 实现固件升级逻辑
            break;
            
        default:
            break;
    }
}
```

---

## 3. 治疗管理器状态机

### 3.1 状态定义

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

### 3.2 状态转换图

```
                    ┌─────────────┐
                    │    IDLE     │
                    └──────┬──────┘
                           │
          ┌────────────────┼────────────────┐
          │                │                │
    探头类型检测      探头类型检测      探头类型检测
          │                │                │
          ▼                ▼                ▼
   ┌────────────┐   ┌────────────┐   ┌────────────┐
   │ULTRASOUND  │   │RADIO_FREQ  │   │ SHOCKWAVE  │
   └──────┬─────┘   └──────┬─────┘   └──────┬─────┘
          │                │                │
          │                │                │
          └────────────────┼────────────────┘
                           │
                    探头断开/错误
                           │
                           ▼
                    ┌─────────────┐
                    │   ERROR     │
                    └──────┬──────┘
                           │
                      错误清除
                           │
                           ▼
                    ┌─────────────┐
                    │    IDLE     │
                    └─────────────┘
```

### 3.3 探头检测逻辑

```c
// 探头类型定义
typedef enum {
    E_IO_WORKING_MODE_IDLE = 0,
    E_IO_WORKING_MODE_RADIO_FREQUENCY,
    E_IO_WORKING_MODE_SHOCK_WAVE,
    E_IO_WORKING_MODE_NEGATIVE_PRESSURE_HEAT,
    E_IO_WORKING_MODE_ULTRASOUND,
    E_IO_WORKING_MODE_MAX
} IODevice_WorkingMode_EnumDef;

// 防抖处理
#define PROBE_STATUS_DEBOUNCE_MS    1000
#define PROBE_STATUS_DEBOUNCE_CNT   (1000 / 10)  // 100次
```

**防抖流程**:
1. 每10ms检测一次探头状态
2. 连续100次检测到相同状态才认为状态改变
3. 状态改变后切换到对应治疗模块

### 3.4 脚踏开关检测

```c
bool eFootSwitchClosed;  // true=按下, false=释放
```

**作用**:
- 控制治疗启动/停止
- 与探头连接状态联合判断
- 安全互锁机制

---

## 4. 超声模块状态机

### 4.1 状态定义

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

### 4.2 状态转换图

```
    ┌─────────────┐
    │    INIT     │
    └──────┬──────┘
           │ 初始化完成
           ▼
    ┌─────────────┐
    │    IDLE     │◄──────────────┐
    └──────┬──────┘               │
           │                      │
      收到启动命令                 │
      脚踏开关按下                 │
           │                      │
           ▼                      │
    ┌─────────────┐               │
    │  WORKING    │               │
    └──────┬──────┘               │
           │                      │
      时间到/停止命令               │
      脚踏开关释放                 │
      错误发生                     │
           │                      │
           ▼                      │
    ┌─────────────┐               │
    │    STOP     │               │
    └──────┬──────┘               │
           │                      │
      关闭输出完成                 │
           │                      │
           ▼                      │
    ┌─────────────┐               │
    │ WAIT_RETURN │───────────────┘
    └─────────────┘
         等待探头返回IDLE
```

### 4.3 状态处理逻辑

#### INIT状态
```c
- 读取EEPROM参数
- 初始化硬件
- 设置默认值
- 转到IDLE
```

#### IDLE状态
```c
- 等待命令
- 监测探头连接
- 响应状态查询
- 收到启动命令 → WORKING
```

#### WORKING状态
```c
- 启动PWM输出
- 监测电流
- 监测温度
- 倒计时
- 检查停止条件:
  * 时间到
  * 收到停止命令
  * 脚踏开关释放
  * 电流异常
  * 温度过高
  * 探头断开
- 满足停止条件 → STOP
```

#### STOP状态
```c
- 关闭PWM输出
- 关闭DAC输出
- 保存状态
- 转到WAIT_RETURN
```

#### WAIT_RETURN状态
```c
- 等待探头返回IDLE状态
- 超时保护
- 收到IDLE确认 → IDLE
```

### 4.4 启动检查

```c
bool App_UltraSound_StartCheck(void)
{
    // 1. 检查探头连接
    if (探头未连接) return false;
    
    // 2. 检查参数有效性
    if (参数无效) return false;
    
    // 3. 检查温度
    if (温度过高) return false;
    
    // 4. 检查脚踏开关
    if (脚踏未按下) return false;
    
    return true;
}
```

---

## 5. 射频模块状态机

### 5.1 状态定义

```c
typedef enum {
    E_RF_RUN_INIT = 0,
    E_RF_RUN_IDLE,
    E_RF_RUN_WORKING,
    E_RF_RUN_STOP,
    E_RF_RUN_WAIT_RETURN,
    E_RF_RUN_MAX
} RF_RunState_EnumDef;
```

### 5.2 状态转换图

与超声模块类似，但有以下特点：

```
    WORKING状态特殊处理:
    ┌─────────────────────────────┐
    │      WORKING                │
    │  ┌─────────────────────┐   │
    │  │ 电压渐增控制         │   │
    │  │ 7V → 目标电压       │   │
    │  └─────────────────────┘   │
    │  ┌─────────────────────┐   │
    │  │ 电流监测 (10ms)     │   │
    │  └─────────────────────┘   │
    │  ┌─────────────────────┐   │
    │  │ 温度监测 (1000ms)   │   │
    │  └─────────────────────┘   │
    └─────────────────────────────┘
```

### 5.3 电压控制策略

```c
// 初始电压: 7V
#define RF_VOLTAGE_INIT_MV  7000

// 目标电压计算
VoltageTarget = RF_VOLTAGE_MIN_MV + (WorkLevel * RF_VOLTAGE_PER_LEVEL_MV)
              = 11000 + (WorkLevel * 950)

// 渐增控制
if (CurrentVoltage < VoltageTarget) {
    CurrentVoltage += STEP;  // 逐步增加
}
```

---

## 6. 冲击波模块状态机

### 6.1 状态定义

```c
typedef enum {
    E_SW_RUN_INIT = 0,
    E_SW_RUN_IDLE,
    E_SW_RUN_WORKING,
    E_SW_RUN_STOP,
    E_SW_RUN_WAIT_RETURN,
    E_SW_RUN_MAX
} SW_RunState_EnumDef;
```

### 6.2 PWM状态机

冲击波模块有独立的PWM控制状态机：

```c
typedef enum {
    E_SW_PWM_STATE_IDLE = 0,
    E_SW_PWM_STATE_ESW_P_HIGH,  // ESW+ 高电平
    E_SW_PWM_STATE_WAIT,        // 等待
    E_SW_PWM_STATE_ESW_N_HIGH,  // ESW-N 高电平
    E_SW_PWM_STATE_MAX
} SW_PWM_State_EnumDef;
```

### 6.3 PWM时序状态机

```
    ┌──────────────┐
    │     IDLE     │
    └──────┬───────┘
           │ 启动
           ▼
    ┌──────────────┐
    │ ESW_P_HIGH   │ (5ms)
    └──────┬───────┘
           │
           ▼
    ┌──────────────┐
    │     WAIT     │ (17ms)
    └──────┬───────┘
           │
           ▼
    ┌──────────────┐
    │ ESW_N_HIGH   │ (3ms + 0.28ms×级别)
    └──────┬───────┘
           │
           │ 周期完成
           └──────► 回到 IDLE (下一个周期)
```

### 6.4 时序参数

```c
#define SW_PWM_ESW_P_HIGH_TIME_MS    5      // ESW+ 高电平时间
#define SW_PWM_ESW_P_WAIT_TIME_MS    17     // 等待时间
#define SW_PWM_ESW_N_BASE_TIME_MS    3      // ESW-N 基础时间
#define SW_PWM_ESW_N_STEP_TIME_MS    0.28f  // ESW-N 步进时间

// 周期计算
cyclePeriodMs = 1000 / freqLevel;  // 频率级别决定周期

// ESW-N 高电平时间
pwmESW_NHighTimeMs = SW_PWM_ESW_N_BASE_TIME_MS + 
                     (workLevel * SW_PWM_ESW_N_STEP_TIME_MS);
```

---

## 7. 负压热疗模块状态机

### 7.1 运行状态定义

```c
typedef enum {
    E_NPH_RUN_INIT = 0,
    E_NPH_RUN_IDLE,
    E_NPH_RUN_PREHEAT,      // 预热状态
    E_NPH_RUN_WORKING,
    E_NPH_RUN_STOP,
    E_NPH_RUN_WAIT_RETURN,
    E_NPH_RUN_MAX
} NPH_RunState_EnumDef;
```

### 7.2 真空控制状态机

```c
typedef enum {
    E_NPH_VACUUM_STATE_IDLE = 0,
    E_NPH_VACUUM_STATE_SUCKING,     // 吸气中
    E_NPH_VACUUM_STATE_MAINTAIN,    // 保持压力
    E_NPH_VACUUM_STATE_RELEASING,   // 释放中
    E_NPH_VACUUM_STATE_MAX
} NPH_Vacuum_State_EnumDef;
```

### 7.3 双层状态机

```
运行状态机:
    ┌──────────────┐
    │     INIT     │
    └──────┬───────┘
           │
           ▼
    ┌──────────────┐
    │     IDLE     │
    └──────┬───────┘
           │
      预热命令
           │
           ▼
    ┌──────────────┐
    │   PREHEAT    │ ◄─────┐
    └──────┬───────┘       │
           │               │
      工作命令              │
           │               │
           ▼               │
    ┌──────────────┐       │
    │   WORKING    │       │
    │  ┌────────┐  │       │
    │  │ 真空   │  │       │
    │  │ 状态机 │  │       │
    │  └────────┘  │       │
    └──────┬───────┘       │
           │               │
      停止命令              │
           │               │
           ▼               │
    ┌──────────────┐       │
    │     STOP     │       │
    └──────┬───────┘       │
           │               │
           ▼               │
    ┌──────────────┐       │
    │ WAIT_RETURN  │───────┘
    └──────────────┘

真空状态机 (仅在WORKING状态):
    ┌──────────────┐
    │     IDLE     │
    └──────┬───────┘
           │
           ▼
    ┌──────────────┐
    │   SUCKING    │ (吸气到目标压力)
    └──────┬───────┘
           │
           ▼
    ┌──────────────┐
    │   MAINTAIN   │ (保持 suck_time)
    └──────┬───────┘
           │
           ▼
    ┌──────────────┐
    │  RELEASING   │ (释放 release_time)
    └──────┬───────┘
           │
           └──────► 回到 SUCKING (循环)
```

### 7.4 预热与工作模式

**预热模式** (PREHEAT):
- 仅加热，不启动真空泵
- 达到预热温度限制后保持
- 等待工作命令

**工作模式** (WORKING):
- 加热 + 真空循环
- 真空状态机控制吸放循环
- 温度控制在工作温度限制

---

## 8. 治疗次数状态机

所有治疗模块共享的次数管理状态：

```c
typedef enum {
    E_TREAT_TIMES_POWER_ON = 0,  // 上电
    E_TREAT_TIMES_WORKING,       // 工作中
    E_TREAT_TIMES_RESET,         // 复位
    E_TREAT_TIMES_WAIT,          // 等待
} Treat_Times_EnumDef;
```

**逻辑**:
```
POWER_ON → 读取EEPROM剩余次数
         ↓
      WORKING → 治疗进行中，次数递减
         ↓
      RESET → 收到复位命令，恢复初始次数
         ↓
      WAIT → 等待下次治疗
```

---

## 9. 状态机设计原则

### 9.1 单一职责
- 每个状态机只负责一个功能域
- 避免状态爆炸

### 9.2 清晰的转换条件
- 每个状态转换都有明确的触发条件
- 避免隐式转换

### 9.3 防抖与滤波
- 探头检测: 1000ms防抖
- 脚踏开关: 建议增加防抖
- 温度/电流: 多次采样平均

### 9.4 安全优先
- 任何异常立即停止输出
- 探头断开立即停止
- 温度过高立即停止
- 电流异常立即停止

### 9.5 状态保存
- 关键状态保存到EEPROM
- 断电恢复机制
- 治疗次数持久化

---

## 10. 状态机调试

### 10.1 日志输出

```c
void System_ChangeMode(System_Mode_EnumDef newMode)
{
    if(newMode != s_SystemMgr.eMode) {
        LOG_I("System mode: %d → %d", s_SystemMgr.eMode, newMode);
        s_SystemMgr.eMode = newMode;
    }
}
```

### 10.2 状态历史记录

建议增加状态历史记录功能：

```c
#define STATE_HISTORY_SIZE  16

typedef struct {
    uint32_t timestamp;
    uint8_t module;
    uint8_t oldState;
    uint8_t newState;
} StateChange_Record_t;

StateChange_Record_t g_StateHistory[STATE_HISTORY_SIZE];
```

### 10.3 状态可视化

通过串口调试助手可以实时查看：
- 当前系统状态
- 治疗模块状态
- 探头连接状态
- 脚踏开关状态

---

## 11. 常见问题

### 11.1 状态卡死

**现象**: 状态机停留在某个状态无法转换

**原因**:
- 转换条件永远不满足
- 死锁
- 硬件故障

**解决**:
- 增加超时机制
- 增加看门狗
- 增加强制复位命令

### 11.2 状态抖动

**现象**: 状态频繁切换

**原因**:
- 防抖不足
- 信号干扰
- 阈值设置不当

**解决**:
- 增加防抖时间
- 硬件滤波
- 调整阈值

### 11.3 状态不同步

**现象**: 上位机显示状态与实际不符

**原因**:
- 通信丢包
- 状态上报不及时
- 协议解析错误

**解决**:
- 增加状态主动上报
- 增加心跳机制
- 增加状态校验

---

## 12. 改进建议

### 12.1 增加状态超时保护

```c
typedef struct {
    uint32_t enterTime;      // 进入状态的时间
    uint32_t maxStayTime;    // 最大停留时间
} StateTimeout_t;
```

### 12.2 增加状态转换回调

```c
typedef void (*StateChangeCallback_t)(uint8_t oldState, uint8_t newState);

void RegisterStateChangeCallback(StateChangeCallback_t callback);
```

### 12.3 状态机框架化

考虑使用状态机框架（如HSM - Hierarchical State Machine）来管理复杂状态。

---

**文档版本**: v1.0  
**最后更新**: 2026-02-26


