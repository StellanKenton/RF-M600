# RF-M600 通信协�??设�?�文�?

## 1. 概述

RF-M600 采用基于串口的自定义二进制通信协�??，用于上位机与�?��?�之间的数据交互�?

**协�??特点**:
- 固定帧头，便于同�?
- CRC16校验，保证数�?完整�?
- �?持�?�模块、�?�命�?
- 小�??�? (Little-Endian)

---

## 2. 协�??帧格�?

### 2.1 帧结�?

```
┌────────�?────────�?─────────�?────────�?─────�?─────────�?──────────�?─────────�?
�? Header �? Header │Direction�? Module �? CMD │Data Len �?   Data   �?  CRC16  �?
�?  0x5A  �?  0xA5  �?  1 Byte �? 1 Byte �?1 Byte�? 1 Byte �? N Bytes  �? 2 Bytes �?
└────────┴────────┴─────────┴────────┴─────┴─────────┴──────────┴─────────�?
  固定      固定     传输方向   模块ID  命令ID 数据长度   数据�?    CRC校验
```

### 2.2 字�?��?�明

| 字�?? | 长度 | 说明 |
|------|------|------|
| Header | 2 Bytes | 固定帧头: 0x5A 0xA5 |
| Direction | 1 Byte | 0x00=主机→�?��??, 0x01=设�?�→主机 |
| Module | 1 Byte | 模块ID (见下�?) |
| CMD | 1 Byte | 命令ID (见下�?) |
| Data Len | 1 Byte | 数据域长�? (0-255) |
| Data | N Bytes | 数据内�?? |
| CRC16 | 2 Bytes | CRC16校验 (仅�?�Data�?) |

### 2.3 模块ID定义

| 模块ID | 名称 | 说明 |
|--------|------|------|
| 0x01 | ULTRASOUND | 超声治疗模块 |
| 0x02 | RADIO_FREQ | 射�?�治疗模�? |
| 0x03 | SHOCKWAVE | 冲击波治疗模�? |
| 0x04 | HEAT | 负压�?疗模�? |

### 2.4 命令ID定义

| 命令ID | 名称 | 说明 |
|--------|------|------|
| 0x00 | GET_STATUS | 获取设�?�状�? |
| 0x01 | SET_WORK_STATE | 设置工作状�? |
| 0x02 | SET_CONFIG | 设置配置参数 |

---

## 3. CRC16校验算法

### 3.1 算法参数

- **多项�?**: 0x8005
- **初�?��?**: 0x0000
- **输入反转**: �? (每字�?)
- **输出反转**: �? (16�?)
- **异或输出**: �?

### 3.2 C�?言实现

```c
uint16_t crc16_compute(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0x0000;
    
    for (uint16_t i = 0; i < len; i++) {
        // 输入反转
        uint8_t r = 0;
        uint8_t b = data[i];
        for (int j = 0; j < 8; j++) {
            r = (r << 1) | (b & 0x01);
            b >>= 1;
        }
        
        crc ^= (r << 8);
        
        // 处理8�?
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = ((crc << 1) ^ 0x8005) & 0xFFFF;
            } else {
                crc = (crc << 1) & 0xFFFF;
            }
        }
    }
    
    // 输出反转
    uint16_t result = 0;
    for (int i = 0; i < 16; i++) {
        result = (result << 1) | (crc & 0x01);
        crc >>= 1;
    }
    
    return result & 0xFFFF;
}
```

---

## 4. 超声模块协�?? (0x01)

### 4.1 获取状�? (0x00)

#### 主机 �? 设�??

| 字�?? | �? | 说明 |
|------|-----|------|
| Header | 5A A5 | 固定帧头 |
| Direction | 00 | 主机→�?��?? |
| Module | 01 | 超声模块 |
| CMD | 00 | 获取状�? |
| Data Len | 01 | 1字节 |
| Data | 00 | Dummy字节 |
| CRC16 | XX XX | CRC校验 |

#### 设�?? �? 主机

| 偏移 | 字�?? | 类型 | 说明 |
|------|------|------|------|
| 0 | work_state | uint8 | 0x00=停�??, 0x01=工作 |
| 1 | frequency | uint16 | 频率 (kHz), 1000-1400 |
| 3 | temp_limit | uint16 | 温度限制, 350-480 (35-48�?) |
| 5 | remain_time | uint16 | 剩余时间 (�?) |
| 7 | work_level | uint8 | 工作级别 0-39 |
| 8 | head_temp | uint16 | 头部温度×10, 0xFFFF=开�?, 0xEEFF=�?�? |
| 10 | conn_state | uint8 | 连接状�? (见下�?) |
| 11 | error_code | uint8 | 错�??�? |
| 12 | remain_treatment_count | uint16 | 剩余治疗次数 |

**总长�?**: 14字节

### 4.2 设置工作状�? (0x01)

#### 主机 �? 设�??

| 偏移 | 字�?? | 类型 | 说明 |
|------|------|------|------|
| 0 | work_state | uint8 | 0x00=停�??, 0x01=开�?, 0x02=复位 |
| 1 | work_time | uint16 | 工作时间 (�?), 最�?3600 |
| 3 | work_level | uint8 | 工作级别 0-39 |

**总长�?**: 4字节

#### 设�?? �? 主机

无响�? (或根�?需要返回确�?)

### 4.3 设置配置 (0x02)

#### 主机 �? 设�??

| 偏移 | 字�?? | 类型 | 说明 |
|------|------|------|------|
| 0 | frequency | uint16 | 频率 (kHz), 1000-1400 |
| 2 | voltage | uint16 | 电压×100 (10mV), 1000-2000 (10-20V) |
| 4 | temp_limit | uint16 | 温度限制, 350-480 |
| 6 | current_high_limit | uint16 | 电流上限 |
| 8 | current_low_limit | uint16 | 电流下限 |
| 10 | remain_treatment_count | uint16 | 剩余治疗次数 |

**总长�?**: 12字节

#### 设�?? �? 主机

| 偏移 | 字�?? | 类型 | 说明 |
|------|------|------|------|
| 0 | freq_result | uint8 | 0x00=成功, 0x01=失败, 0x02=超限 |
| 1 | voltage_result | uint8 | 配置结果 |
| 2 | temp_result | uint8 | 配置结果 |
| 3 | current_high_result | uint8 | 配置结果 |
| 4 | current_low_result | uint8 | 配置结果 |
| 5 | remain_count_result | uint8 | 配置结果 |

**总长�?**: 6字节

---

## 5. 射�?�模块协�? (0x02)

### 5.1 获取状�? (0x00)

#### 设�?? �? 主机

| 偏移 | 字�?? | 类型 | 说明 |
|------|------|------|------|
| 0 | work_state | uint8 | 0x00=停�??, 0x01=工作 |
| 1 | temp_limit | uint16 | 温度限制, 350-480 |
| 3 | remain_time | uint16 | 剩余时间 (�?) |
| 5 | work_level | uint8 | 工作级别 0-20 |
| 6 | head_temp | uint16 | 头部温度×10 |
| 8 | conn_state | uint8 | 连接状�? |
| 9 | error_code | uint8 | 错�??�? |
| 10 | remain_treatment_count | uint16 | 剩余治疗次数 |

**总长�?**: 12字节

### 5.2 设置工作状�? (0x01)

#### 主机 �? 设�??

| 偏移 | 字�?? | 类型 | 说明 |
|------|------|------|------|
| 0 | work_state | uint8 | 0x00=停�??, 0x01=开�?, 0x02=复位 |
| 1 | work_time | uint16 | 工作时间 (�?) |
| 3 | work_level | uint8 | 工作级别 0-20 |

**总长�?**: 4字节

### 5.3 设置配置 (0x02)

#### 主机 �? 设�??

| 偏移 | 字�?? | 类型 | 说明 |
|------|------|------|------|
| 0 | temp_limit | uint16 | 温度限制, 350-480 |
| 2 | current_high_limit | uint16 | 电流上限 |
| 4 | current_low_limit | uint16 | 电流下限 |
| 6 | remain_treatment_count | uint16 | 剩余治疗次数 |

**总长�?**: 8字节

#### 设�?? �? 主机

| 偏移 | 字�?? | 类型 | 说明 |
|------|------|------|------|
| 0 | temp_result | uint8 | 配置结果 |
| 1 | current_high_result | uint8 | 配置结果 |
| 2 | current_low_result | uint8 | 配置结果 |
| 3 | remain_count_result | uint8 | 配置结果 |

**总长�?**: 4字节

---

## 6. 冲击波模块协�? (0x03)

### 6.1 获取状�? (0x00)

#### 设�?? �? 主机

| 偏移 | 字�?? | 类型 | 说明 |
|------|------|------|------|
| 0 | work_state | uint8 | 0x00=停�??, 0x01=工作 |
| 1 | frequency | uint8 | 频率级别 1-16 |
| 2 | remain_time | uint16 | 剩余时间 (�?) |
| 4 | work_level | uint8 | 工作级别 0-26 |
| 5 | head_temp | uint16 | 头部温度×10 |
| 7 | conn_state | uint8 | 连接状�? |
| 8 | error_code | uint8 | 错�??�? |
| 9 | remain_treatment_count | uint16 | 剩余治疗次数 |

**总长�?**: 11字节

### 6.2 设置工作状�? (0x01)

#### 主机 �? 设�??

| 偏移 | 字�?? | 类型 | 说明 |
|------|------|------|------|
| 0 | work_state | uint8 | 0x00=停�??, 0x01=开�?, 0x02=复位 |
| 1 | work_time | uint16 | 工作时间 (�?) |
| 3 | work_level | uint8 | 工作级别 0-26 |
| 4 | frequency | uint8 | 频率级别 0-16 |

**总长�?**: 5字节

### 6.3 设置配置 (0x02)

#### 主机 �? 设�??

| 偏移 | 字�?? | 类型 | 说明 |
|------|------|------|------|
| 0 | temp_limit | uint16 | 温度限制 |
| 2 | esw_p_current_high | uint16 | ESW-P电流上限 |
| 4 | esw_p_current_low | uint16 | ESW-P电流下限 |
| 6 | remain_treatment_count | uint16 | 剩余治疗次数 |
| 8 | esw_n_current_high | uint16 | ESW-N电流上限 |
| 10 | esw_n_current_low | uint16 | ESW-N电流下限 |

**总长�?**: 12字节

#### 设�?? �? 主机

| 偏移 | 字�?? | 类型 | 说明 |
|------|------|------|------|
| 0 | temp_result | uint8 | 配置结果 |
| 1 | esw_p_high_result | uint8 | 配置结果 |
| 2 | esw_p_low_result | uint8 | 配置结果 |
| 3 | remain_count_result | uint8 | 配置结果 |
| 4 | esw_n_high_result | uint8 | 配置结果 |
| 5 | esw_n_low_result | uint8 | 配置结果 |

**总长�?**: 6字节

---

## 7. �?疗模块协�? (0x04)

### 7.1 获取状�? (0x00)

#### 设�?? �? 主机

| 偏移 | 字�?? | 类型 | 说明 |
|------|------|------|------|
| 0 | work_state | uint8 | 0x00=停�??, 0x01=工作 |
| 1 | temp_limit | uint16 | 温度限制 |
| 3 | remain_heat_time | uint16 | 剩余加热时间 (�?) |
| 5 | suck_time | uint16 | 吸合时间 (10ms单位) |
| 7 | release_time | uint16 | 释放时间 (10ms单位) |
| 9 | pressure | uint8 | 压力 (KPa) |
| 10 | head_temp | uint16 | 头部温度×10 |
| 12 | preheat_state | uint8 | 预热状�? |
| 13 | preheat_temp_limit | uint16 | 预热温度限制 |
| 15 | remain_preheat_time | uint16 | 剩余预热时间 (�?) |
| 17 | conn_state | uint8 | 连接状�? |
| 18 | error_code | uint8 | 错�??�? |
| 19 | remain_treatment_count | uint16 | 剩余治疗次数 |
| 21 | current_pressure_kpa | int16 | 实时负压值 (kPa)，仅负压探头有效，旧版本可能不包含 |

**总长�?**: 23字节（兼容旧版21字节）

### 7.2 设置工作状�? (0x01)

#### 主机 �? 设�??

| 偏移 | 字�?? | 类型 | 说明 |
|------|------|------|------|
| 0 | work_state | uint8 | 0x00=停�??, 0x01=开�?, 0x02=复位 |
| 1 | work_time | uint16 | 工作时间 (�?) |
| 3 | pressure | uint8 | 压力 10-100 KPa |
| 4 | suck_time | uint16 | 吸合时间 (10ms单位) |
| 6 | release_time | uint16 | 释放时间 (10ms单位) |
| 8 | temp_limit | uint16 | 温度限制 |

**总长�?**: 10字节

### 7.3 设置配置 (0x02)

#### 主机 �? 设�??

| 偏移 | 字�?? | 类型 | 说明 |
|------|------|------|------|
| 0 | preheat_state | uint8 | 0x00=停�??, 0x01=开�? |
| 1 | work_time | uint16 | 工作时间 (�?) |
| 3 | temp_limit | uint16 | 温度限制 |
| 5 | preheat_temp_limit | uint16 | 预热温度限制 |
| 7 | remain_treatment_count | uint16 | 剩余治疗次数 |

**总长�?**: 9字节

#### 设�?? �? 主机

| 偏移 | 字�?? | 类型 | 说明 |
|------|------|------|------|
| 0 | preheat_state_result | uint8 | 配置结果 |
| 1 | work_time_result | uint8 | 配置结果 |
| 2 | temp_limit_result | uint8 | 配置结果 |
| 3 | preheat_temp_result | uint8 | 配置结果 |
| 4 | remain_count_result | uint8 | 配置结果 |

**总长�?**: 5字节

---

## 8. 连接状态定�?

| �? | 说明 |
|----|------|
| 0x00 | 头部连接，脚踏关�? |
| 0x01 | 头部连接，脚踏打开 |
| 0x10 | 头部�?开，脚踏关�? |
| 0x11 | 头部�?开，脚踏打开 |

---

## 9. 温度/参数错�??�?

| �? | 说明 |
|----|------|
| 0xFFFF | NTC开�? �? 参数超限 |
| 0xEEFF | NTC�?�? |

---

## 10. 配置结果�?

| �? | 说明 |
|----|------|
| 0x00 | 配置成功 |
| 0x01 | 配置失败 |
| 0x02 | 参数超限 |

---

## 11. 通信示例

### 11.1 获取超声模块状�?

**主机发�?**:
```
5A A5 00 01 00 01 00 [CRC_L] [CRC_H]
```

**设�?�响�?**:
```
5A A5 01 01 00 0E 
01              // work_state = 工作�?
B0 04           // frequency = 1200 kHz
90 01           // temp_limit = 400 (40�?)
3C 00           // remain_time = 60�?
0A              // work_level = 10
F4 01           // head_temp = 500 (50�?)
00              // conn_state = 头部连接，脚踏关�?
00              // error_code = 无错�?
64 00           // remain_treatment_count = 100
[CRC_L] [CRC_H]
```

### 11.2 设置射�?�工作状�?

**主机发�?**:
```
5A A5 00 02 01 04
01              // work_state = 开�?
3C 00           // work_time = 60�?
0F              // work_level = 15
[CRC_L] [CRC_H]
```

---

## 12. Python实现示例

```python
def build_packet(direction, module, cmd, data_bytes):
    """构建数据�?"""
    packet = bytearray()
    packet.append(0x5A)  # Header 0
    packet.append(0xA5)  # Header 1
    packet.append(direction)
    packet.append(module)
    packet.append(cmd)
    packet.append(len(data_bytes))
    packet.extend(data_bytes)
    
    # 计算CRC16
    crc = crc16_compute(data_bytes)
    packet.append(crc & 0xFF)
    packet.append((crc >> 8) & 0xFF)
    
    return bytes(packet)
```

---

## 13. 注意事项

1. **字节�?**: 所有�?�字节数�?采用小�??�? (Little-Endian)
2. **CRC校验**: 仅�?�Data域�?�算，不包括帧头、方向、模块、命令、长度和CRC�?�?
3. **超时处理**: 建�??命令超时时间设置�?1000ms
4. **重传机制**: 建�??最多重�?3�?
5. **温度单位**: 温度值需除以10得到实际温度 (�?)
6. **时间单位**: 
   - work_time: �?
    - suck_time/release_time (�?�?): 设置工作状�?(0x01)与获取状�?(0x00)均使�?10ms

---

**文档版本**: v1.0  
**最后更�?**: 2026-02-26


