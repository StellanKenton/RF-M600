/************************************************************************************
* @file     : app_radiofreq.h
* @brief    : Radio Frequency treatment module header file
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef APP_RADIOFREQ_H
#define APP_RADIOFREQ_H

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

/* 射�?�工作�?�率固定�?1MHz */
#define RF_FREQUENCY_KHZ           1000        ///< 射�?�工作�?�率 (kHz)
#define RF_WORK_LEVEL_MAX          20          ///< 最大档�? (0-20)
#define RF_VOLTAGE_MIN_MV          11000       ///< 最小工作电�? (11V = 11000mV)
#define RF_VOLTAGE_MAX_MV          30000       ///< 最大工作电�? (30V = 30000mV)
#define RF_VOLTAGE_INIT_MV         7000        ///< 初�?�工作电�? (7V = 7000mV)
#define RF_CURRENT_THRESHOLD_MV    500         ///< 电流阈�? (0.5V = 500mV)
#define RF_CURRENT_MONITOR_PERIOD_MS   10      ///< 电流监控周期 (10ms)
#define RF_TEMP_MONITOR_PERIOD_MS      1000    ///< 温度监控周期 (1s)

/* 档位到电压的映射�?1-20档位对应11-30V */
#define RF_VOLTAGE_PER_LEVEL_MV    ((RF_VOLTAGE_MAX_MV - RF_VOLTAGE_MIN_MV) / RF_WORK_LEVEL_MAX)

typedef enum
{
    E_RF_RUN_INIT = 0,
    E_RF_RUN_IDLE,
    E_RF_RUN_WORKING,
    E_RF_RUN_STOP,
    E_RF_RUN_WAIT_RETURN,
    E_RF_RUN_MAX,
} RF_RunState_EnumDef;

typedef enum {
    E_RF_ERROR_NONE = 0,
    E_RF_ERROR_PROBE_NOT_CONNECTED,
    E_RF_ERROR_READ_PARAMS_FAILED,
    E_RF_ERROR_INVALID_PARAMS,
    E_RF_ERROR_CURRENT_TOO_LOW,
    E_RF_ERROR_TEMP_TOO_HIGH,
    E_RF_ERROR_MAX,
} RF_ErrorCode_EnumDef;

typedef struct
{
    RF_RunState_EnumDef runState;
    bool isWaitReturn;
    uint16_t Voltage;              ///< 当前工作电压 (mV)
    uint16_t VoltageTarget;        ///< �?标工作电�? (mV)，根�?档位计算
    uint16_t CurrentHigh;         ///< 工作电流上限 (mV)
    uint16_t CurrentLow;          ///< 工作电流下限 (mV)
    uint16_t TempLimit;            ///< 治疗头温度上�? (0.1°C)
    uint16_t TreatTimes;           ///< 剩余�?治疗次数
    
    uint8_t WorkLevel;             ///< 工作档位 (0-20)
    uint16_t HeadTemp;             ///< 治疗头温�? (0.1°C)
    uint16_t RemainTime;           ///< 剩余工作时间 (�?)
    
    uint8_t ConnState;
    uint8_t ErrorCode;
    bool FootSwitchStatus;
    IODevice_WorkingMode_EnumDef probeStatus;
    IODevice_WorkingMode_EnumDef preProbeStatus;
    
    RF_TreatParams_t TreatParams;
    RF_TransData_t Trans;
    
    /* 监控定时�? */
    uint32_t lastCurrentMonitorTime;   ///< 上�?�电流监控时�?
    uint32_t lastTempMonitorTime;      ///< 上�?�温度监控时�?
} RF_CtrlInfo_t;

void App_RadioFreq_Init(void);
void App_RadioFreq_Process(void);
bool App_RadioFreq_StartCheck(void);
void App_RadioFreq_SetWorkParams(void);
RF_GetStatus_Reply_t *App_RadioFreq_GetStatus(void);
RF_RunState_EnumDef App_RadioFreq_GetRunState(void);
void App_RadioFreq_ChangeState(RF_RunState_EnumDef newState);

#ifdef __cplusplus
}
#endif
#endif  // APP_RADIOFREQ_H
/**************************End of file********************************/
