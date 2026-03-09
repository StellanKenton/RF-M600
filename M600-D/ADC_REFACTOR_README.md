# ADC模块重构说明

## 重构概述

已完成 `bsp_adc` 和 `drv_adc` 模块的重构，架构更加清晰，职责分离明确。

## 模块职责

### BSP层 (bsp_adc.h/c)
- **职责**：硬件抽象层，负责获取原始ADC值
- **核心函数**：
  - `BSP_ADC_Init()` - 初始化ADC模块
  - `BSP_ADC_ReadChannel(BSP_ADC_Channel_t ch)` - 读取原始ADC值 (0-4095)

### DRV层 (drv_adc.h/c)
- **职责**：驱动层，提供电压值和处理后的物理量
- **核心函数**：
  1. `DRV_ADC_ReadRaw(BSP_ADC_Channel_t ch)` - 读取原始ADC值
  2. `DRV_ADC_ReadVoltage(BSP_ADC_Channel_t ch)` - 读取电压值 (mV)
  3. `DRV_ADC_GetProcessedValue(BSP_ADC_Channel_t ch)` - **通用函数**，根据通道类型返回处理后的值

## 通道定义

```c
typedef enum {
    BSP_ADC_CH_US_I = 0,       /* 超声电流 - ADC1_IN0  PA0 */
    BSP_ADC_CH_RF_I,           /* 射频电流 - ADC1_IN1  PA1 */
    BSP_ADC_CH_Heat_REF02,     /* 加热参考2 - ADC1_IN5  PA5 */
    BSP_ADC_CH_Heat_REF01,     /* 加热参考1/主板NTC - ADC1_IN6  PA6 */
    BSP_ADC_CH_ESW_U,          /* ESW电压 - ADC1_IN8  PB0 */
    BSP_ADC_CH_ESW_I,          /* ESW电流 - ADC1_IN9  PB1 */
    BSP_ADC_CH_HP_PRE,         /* 高压预压 - ADC1_IN12 PC2 */
    BSP_ADC_CH_HAND_NTC,       /* 手柄NTC - ADC1_IN13 PC3 */
    BSP_ADC_CH_VOUT,           /* 输出电压 - ADC1_IN15 PC5 */
    BSP_ADC_CH_MAX
} BSP_ADC_Channel_t;
```

## 使用示例

### 新版接口（推荐）

```c
#include "drv_adc.h"

// 读取原始ADC值
uint16_t raw = DRV_ADC_ReadRaw(BSP_ADC_CH_US_I);

// 读取电压值
uint16_t voltage_mv = DRV_ADC_ReadVoltage(BSP_ADC_CH_ESW_U);

// 读取处理后的值（通用函数）
uint16_t temp = DRV_ADC_GetProcessedValue(BSP_ADC_CH_HAND_NTC);
if (temp == DRV_ADC_NTC_FAULT_OPEN) {
    // NTC开路
} else if (temp == DRV_ADC_NTC_FAULT_SHORT) {
    // NTC短路
} else {
    // 实际温度 = (temp / 10.0) - 40
    // 例如：temp = 400 表示 0°C
}

// 读取电流值
uint16_t current_ma = DRV_ADC_GetProcessedValue(BSP_ADC_CH_US_I);

// 读取VOUT补偿后的电压
uint16_t vout_mv = DRV_ADC_GetProcessedValue(BSP_ADC_CH_VOUT);
```

### 旧版接口（兼容）

为了保持向后兼容，所有旧版函数仍然可用：

```c
// 旧版枚举和函数仍然可用
uint16_t raw = Drv_ADC_ReadChannel(E_ADC_CHANNEL_US_I);
uint32_t voltage = Drv_ADC_ReadVoltage(E_ADC_CHANNEL_ESW_U);
uint16_t temp = Drv_ADC_GetNTCValue(E_NTC_HAND);
uint16_t value = Drv_ADC_GetRealValue(E_ADC_CHANNEL_HP_PRE);
uint16_t vout = Drv_GetADCVout();
```

## 处理后的值说明

`DRV_ADC_GetProcessedValue()` 根据通道类型返回不同的值：

| 通道类型 | 返回值说明 |
|---------|-----------|
| **NTC温度通道** | 温度值：(温度+40)*10<br>范围：0~1450 对应 -40~105°C<br>故障码：0xFFFF(开路) / 0xFFEE(短路) |
| **电流通道** | 电流值 (mA) |
| **电压通道** | 电压值 (mV) |
| **VOUT通道** | 补偿后的输出电压 (mV)<br>Vout = 15.3529 × Vsample |

## 温度值转换

```c
uint16_t temp_raw = DRV_ADC_GetProcessedValue(BSP_ADC_CH_HAND_NTC);

if (temp_raw == DRV_ADC_NTC_FAULT_OPEN) {
    printf("NTC开路\n");
} else if (temp_raw == DRV_ADC_NTC_FAULT_SHORT) {
    printf("NTC短路\n");
} else {
    // 转换为实际温度（°C）
    float temp_celsius = (temp_raw / 10.0f) - 40.0f;
    printf("温度: %.1f°C\n", temp_celsius);
}
```

## 数据类型

所有函数返回值均为 `uint16_t`，符合要求。

## 兼容性

- ✅ 完全兼容旧代码
- ✅ 所有旧版函数和枚举仍然可用
- ✅ 无需修改现有应用层代码
- ✅ 新代码推荐使用新版接口

## 文件清单

- `M600-D/BSP/bsp_adc.h` - BSP层头文件
- `M600-D/BSP/bsp_adc.c` - BSP层实现
- `M600-D/User/DRV/drv_adc.h` - DRV层头文件
- `M600-D/User/DRV/drv_adc.c` - DRV层实现

## 注意事项

1. 电流通道的转换系数需要根据实际硬件电路调整（当前为1mV=1mA）
2. NTC温度表来自规格书，适用于10K NTC
3. VOUT分压系数为15.3529，根据硬件设计确定
4. 所有ADC采样使用DMA双缓冲机制，确保数据一致性
