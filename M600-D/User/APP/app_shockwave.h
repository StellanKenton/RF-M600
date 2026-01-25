/************************************************************************************
* @file     : app_shockwave.h
* @brief    : Shock Wave treatment module header file
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef APP_SHOCKWAVE_H
#define APP_SHOCKWAVE_H

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

/* 冲击波工作参数 */
#define SW_WORK_LEVEL_MAX          26          ///< 最大档位 (0-26)
#define SW_FREQ_LEVEL_MAX          16          ///< 最大频率档位 (1-16)
#define SW_WORK_POINT_MAX          10000       ///< 最大工作点数
#define SW_VOLTAGE_THRESHOLD_MV    3000        ///< 电压报警阈值 (3V = 3000mV)
#define SW_TEMP_MONITOR_PERIOD_MS  10          ///< 温度监控周期 (10ms)

/* PWM时序参数 */
#define SW_PWM_ESW_P_HIGH_TIME_MS     5       ///< PWM_ESW+高电平时间 (5ms)
#define SW_PWM_ESW_P_WAIT_TIME_MS     17      ///< PWM_ESW+后等待时间 (17ms)
#define SW_PWM_ESW_N_BASE_TIME_MS     3       ///< PWM_ESW-基础高电平时间 (3ms)
#define SW_PWM_ESW_N_STEP_TIME_MS     0.28f   ///< PWM_ESW-每档增加时间 (0.28ms)

typedef enum
{
    E_SW_RUN_INIT = 0,
    E_SW_RUN_IDLE,
    E_SW_RUN_WORKING,
    E_SW_RUN_STOP,
    E_SW_RUN_MAX,
} SW_RunState_EnumDef;

typedef enum {
    E_SW_ERROR_NONE = 0,
    E_SW_ERROR_PROBE_NOT_CONNECTED,
    E_SW_ERROR_READ_PARAMS_FAILED,
    E_SW_ERROR_INVALID_PARAMS,
    E_SW_ERROR_CURRENT_ESW_P_LOW,
    E_SW_ERROR_CURRENT_ESW_N_LOW,
    E_SW_ERROR_VOLTAGE_LOW,
    E_SW_ERROR_TEMP_TOO_HIGH,
    E_SW_ERROR_MAX,
} SW_ErrorCode_EnumDef;

typedef enum
{
    E_SW_PWM_STATE_IDLE = 0,
    E_SW_PWM_STATE_ESW_P_HIGH,
    E_SW_PWM_STATE_WAIT,
    E_SW_PWM_STATE_ESW_N_HIGH,
    E_SW_PWM_STATE_MAX,
} SW_PWM_State_EnumDef;

typedef struct
{
    SW_RunState_EnumDef runState;
    SW_PWM_State_EnumDef pwmState;
    
    uint16_t TempLimit;            ///< 治疗头温度上限 (0.1°C)
    uint16_t TreatTimes;           ///< 剩余可治疗次数
    uint16_t CurrentHigh_ESW_P;    ///< PWM_ESW+工作电流上限 (mV)
    uint16_t CurrentLow_ESW_P;     ///< PWM_ESW+工作电流下限 (mV)
    uint16_t CurrentHigh_ESW_N;   ///< PWM_ESW-工作电流上限 (mV)
    uint16_t CurrentLow_ESW_N;     ///< PWM_ESW-工作电流下限 (mV)
    
    uint8_t WorkLevel;             ///< 工作档位 (0-26)
    uint8_t FreqLevel;             ///< 工作频率档位 (1-16)
    uint16_t RemainPoints;         ///< 剩余工作点数
    uint16_t HeadTemp;             ///< 治疗头温度 (0.1°C)
    
    uint8_t ConnState;
    uint8_t ErrorCode;
    bool FootSwitchStatus;
    IODevice_WorkingMode_EnumDef probeStatus;
    
    SW_TreatParams_t TreatParams;
    SW_TransData_t Trans;
    
    /* PWM控制定时器 */
    uint32_t pwmStateStartTime;    ///< PWM状态开始时间 (ms)
    uint32_t cycleStartTime;       ///< 周期开始时间 (ms)
    uint32_t cyclePeriodMs;        ///< 周期时间 (ms)，根据频率档位计算
    uint32_t pwmESW_NHighTimeMs;   ///< PWM_ESW-高电平时间 (ms)
    
    /* 监控定时器 */
    uint32_t lastTempMonitorTime;  ///< 上次温度监控时间
} SW_CtrlInfo_t;

void App_Shockwave_Init(void);
void App_Shockwave_Process(void);
bool App_Shockwave_StartCheck(void);
void App_Shockwave_SetWorkParams(void);

#ifdef __cplusplus
}
#endif
#endif  // APP_SHOCKWAVE_H
/**************************End of file********************************/
