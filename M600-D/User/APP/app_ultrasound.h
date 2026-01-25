/************************************************************************************
* @file     : app_ultrasound.h
* @brief    : 
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef APP_ULTRASOUND_H
#define APP_ULTRASOUND_H

#include <string.h>
#include <stdbool.h>
#include "stdint.h"

#ifdef __cplusplus
#include <iostream>
extern "C" {
#endif

#include "app_comm.h"
#include "app_memory.h"
#include "drv_iodevice.h"


/* 档位到脉冲重复时间的映射：20ms基准，0.5ms步进 */
/* 档位0对应20ms，档位39对应0.5ms (20ms - 39*0.5ms = 0.5ms) */
#define PULSE_REPEAT_TIME_BASE_MS    20      ///< 基准脉冲重复时间 (ms)
#define PULSE_REPEAT_TIME_STEP_MS    0.5f    ///< 每档步进 (ms)
#define PULSE_REPEAT_TIME_MIN_MS     0.5f    ///< 最小脉冲重复时间 (ms)
#define PULSE_REPEAT_TIME_MAX_MS     20      ///< 最大脉冲重复时间 (ms)
#define WORK_LEVEL_MAX               40      ///< 最大档位 (0-39共40个档位)

/* 电压调节限制 */
#define VOLTAGE_ADJUST_LIMIT_MV       2000    ///< 电压调节限制 ±2V = 2000mV

typedef enum
{
    E_US_RUN_INIT = 0,
    E_US_RUN_IDLE,
    E_US_RUN_WORKING,
    E_US_RUN_STOP,
    E_US_RUN_MAX,
} US_RunState_EnumDef;


typedef enum {
    E_US_ERROR_NONE = 0,
    E_US_ERROR_PROBE_NOT_CONNECTED,
    E_US_ERROR_READ_PARAMS_FAILED,
    E_US_ERROR_INVALID_PARAMS,
    E_US_ERROR_CURRENT_TOO_HIGH,
    E_US_ERROR_CURRENT_TOO_LOW,
    E_US_ERROR_TEMP_TOO_HIGH,
    E_US_ERROR_TEMP_TOO_LOW,
    E_US_ERROR_VOLTAGE_OVER_LIMIT,
    E_US_ERROR_MAX,
}Ultrasound_ErrorCode_EnumDef;

typedef struct
{
    US_RunState_EnumDef runState;
    uint16_t Voltage;              ///< 工作电压 (mV)
    uint16_t VoltageBase;          ///< 基础工作电压 (mV)，用于超限检测
    uint16_t CurrentHigh;
    uint16_t CurrentLow;
    uint16_t Frequency;
    uint16_t TempLimit;
    uint16_t TreatTimes;  

    uint8_t WorkLevel;
    uint16_t HeadTemp;
    uint16_t RemainTime;
    
    uint8_t ConnState;
    uint8_t ErrorCode;
    bool FootSwitchStatus;
    IODevice_WorkingMode_EnumDef probeStatus;
    
    US_TreatParams_t TreatParams;
    UltraSound_TransData_t Trans;
} US_CtrlInfo_t;


void App_Ultrasound_Init(void);
void App_Ultrasound_Process(void);
bool App_UltraSound_StartCheck(void);
void App_UltraSound_SetWorkParams(void);


#ifdef __cplusplus
}
#endif
#endif  // APP_ULTRASOUND_H
/**************************End of file********************************/
