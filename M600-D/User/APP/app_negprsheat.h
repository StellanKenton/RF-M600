/************************************************************************************
* @file     : app_negprsheat.h
* @brief    : Negative Pressure Heat treatment module header file
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef APP_NEGPRSHEAT_H
#define APP_NEGPRSHEAT_H

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

/* 负压加热工作参数 */
#define NPH_WORK_TIME_MAX           3600        ///< 最大工作时间 (秒)
#define NPH_PRESSURE_MIN_KPA        10          ///< 最小负压大小 (10KPa)
#define NPH_PRESSURE_MAX_KPA        100         ///< 最大负压大小 (100KPa)
#define NPH_SUCK_TIME_MIN_MS        100         ///< 最小负压吸时间 (0.1s = 100ms)
#define NPH_SUCK_TIME_MAX_MS        60000       ///< 最大负压吸时间 (60s = 60000ms)
#define NPH_RELEASE_TIME_MIN_MS     100         ///< 最小负压放时间 (0.1s = 100ms)
#define NPH_RELEASE_TIME_MAX_MS     60000       ///< 最大负压放时间 (60s = 60000ms)
#define NPH_TEMP_MONITOR_PERIOD_MS  10          ///< 温度监控周期 (10ms)
#define NPH_TEMP_ERROR_THRESHOLD    650         ///< 温度错误阈值 (65℃ = 650 * 0.1°C)
#define NPH_TEMP_ERROR_TIME_MS      2000        ///< 温度错误检测时间 (2s = 2000ms)

typedef enum
{
    E_NPH_RUN_INIT = 0,
    E_NPH_RUN_IDLE,
    E_NPH_RUN_PREHEAT,
    E_NPH_RUN_WORKING,
    E_NPH_RUN_STOP,
    E_NPH_RUN_MAX,
} NPH_RunState_EnumDef;

typedef enum {
    E_NPH_ERROR_NONE = 0,
    E_NPH_ERROR_PROBE_NOT_CONNECTED,
    E_NPH_ERROR_READ_PARAMS_FAILED,
    E_NPH_ERROR_INVALID_PARAMS,
    E_NPH_ERROR_TEMP_TOO_HIGH,
    E_NPH_ERROR_TEMP_RISE_TOO_FAST,
    E_NPH_ERROR_TEMP_SENSOR_ERROR,
    E_NPH_ERROR_MAX,
} NPH_ErrorCode_EnumDef;

typedef enum
{
    E_NPH_VACUUM_STATE_IDLE = 0,
    E_NPH_VACUUM_STATE_SUCKING,
    E_NPH_VACUUM_STATE_MAINTAIN,
    E_NPH_VACUUM_STATE_RELEASING,
    E_NPH_VACUUM_STATE_MAX,
} NPH_Vacuum_State_EnumDef;

typedef struct
{
    NPH_RunState_EnumDef runState;
    NPH_Vacuum_State_EnumDef vacuumState;
    
    uint16_t TempLimit;            ///< 工作温度上限 (0.1°C)
    uint16_t TreatTimes;           ///< 剩余可治疗次数
    bool PreheatEnable;            ///< 预热功能是否开启
    uint16_t PreheatTempLimit;    ///< 预热温度上限 (0.1°C)
    uint16_t PreheatTime;         ///< 预热时间 (秒)
    
    uint16_t WorkTempLimit;        ///< 当前工作温度上限 (0.1°C)
    uint16_t RemainTime;           ///< 剩余工作时间 (秒)
    uint8_t Pressure;             ///< 负压大小 (10-100 KPa，发送正值)
    uint16_t SuckTime;             ///< 负压吸时间 (0.1s单位，实际为100ms单位)
    uint16_t ReleaseTime;          ///< 负压放时间 (0.1s单位，实际为100ms单位)
    uint16_t HeadTemp;             ///< 治疗头温度 (0.1°C)
    
    uint8_t ConnState;
    uint8_t ErrorCode;
    bool FootSwitchStatus;
    IODevice_WorkingMode_EnumDef probeStatus;
    
    NPH_TreatParams_t TreatParams;
    Heat_TransData_t Trans;
    
    /* 温度控制 */
    bool heatControlActive;        ///< 加热控制是否激活
    uint32_t lastTempMonitorTime;  ///< 上次温度监控时间
    uint32_t tempErrorStartTime;   ///< 温度错误开始时间
    uint16_t lastTemp;             ///< 上次温度值
    
    /* 负压控制 */
    uint32_t vacuumStateStartTime; ///< 负压状态开始时间
    uint16_t targetPressure;       ///< 目标负压值 (KPa，采样电压转换)
    uint16_t currentPressure;      ///< 当前负压值 (KPa，采样电压转换)
    uint32_t suckStartTime;        ///< 吸气开始时间
    uint32_t maintainStartTime;    ///< 维持开始时间
    uint32_t releaseStartTime;     ///< 放气开始时间
    bool motorState;               ///< 电机状态 (true=运行, false=停止)
} NPH_CtrlInfo_t;

void App_NegPrsHeat_Init(void);
void App_NegPrsHeat_Process(void);
bool App_NegPrsHeat_StartCheck(void);
void App_NegPrsHeat_SetWorkParams(void);

#ifdef __cplusplus
}
#endif
#endif  // APP_NEGPRSHEAT_H
/**************************End of file********************************/
