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

/* ???????? */
#define NPH_WORK_TIME_MAX           3600        ///< ??????? (??)
#define NPH_PRESSURE_MIN_KPA        10          ///< ??????? (10KPa)
#define NPH_PRESSURE_MAX_KPA        100         ///< ??????? (100KPa)
#define NPH_SUCK_TIME_MIN_MS        100         ///< ??????? (0.1s = 100ms)
#define NPH_SUCK_TIME_MAX_MS        60000       ///< ??????? (60s = 60000ms)
#define NPH_RELEASE_TIME_MIN_MS     100         ///< ??????? (0.1s = 100ms)
#define NPH_RELEASE_TIME_MAX_MS     60000       ///< ??????? (60s = 60000ms)
#define NPH_TEMP_MONITOR_PERIOD_MS  10          ///< ?????? (10ms)
#define NPH_TEMP_ERROR_THRESHOLD    650         ///< ????????? (65?? = 650 * 0.1°C)
#define NPH_TEMP_ERROR_TIME_MS      2000        ///< ??????????? (2s = 2000ms)

typedef enum
{
    E_NPH_RUN_INIT = 0,
    E_NPH_RUN_IDLE,
    E_NPH_RUN_PREHEAT,
    E_NPH_RUN_WORKING,
    E_NPH_RUN_STOP,
    E_NPH_RUN_WAIT_RETURN,
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
    bool isWaitReturn;

    uint16_t TempLimit;            ///< ?????? (0.1°C)
    uint16_t TreatCounts;           ///< ????????
    bool PreheatEnable;            ///< ??????????
    uint16_t PreheatTempLimit;    ///< ?????? (0.1°C)
    uint16_t PreheatTime;         ///< ???? (??)
    
    uint16_t WorkTempLimit;        ///< ???????? (0.1°C)
    uint16_t TreatRemainTimes;           ///< ?????? (??)
    uint8_t Pressure;             ///< ???? (10-100 KPa????????)
    uint16_t SuckTime;             ///< ?????? (0.1s??????100ms??)
    uint16_t ReleaseTime;          ///< ?????? (0.1s??????100ms??)
    uint16_t HeadTemp;             ///< ?????? (0.1°C)
    
    uint8_t ErrorCode;
    
    NPH_TreatParams_t TreatParams;
    Heat_TransData_t Trans;
    
    /* ???? */
    bool heatControlActive;        ///< ??????????
    uint32_t lastTempMonitorTime;  ///< ???????????
    uint32_t tempErrorStartTime;   ///< ???????????
    uint16_t lastTemp;             ///< ????????
    
    /* ???? */
    uint32_t vacuumStateStartTime; ///< ?????????
    uint16_t targetPressure;       ///< ??????? (KPa????????)
    uint16_t currentPressure;      ///< ?????? (KPa????????)
    uint32_t suckStartTime;        ///< ???????
    uint32_t maintainStartTime;    ///< ???????
    uint32_t releaseStartTime;     ///< ???????
    bool motorState;               ///< ????? (true=????, false=????)
} NPH_CtrlInfo_t;

void App_NegPrsHeat_Init(void);
void App_NegPrsHeat_Process(void);
bool App_NegPrsHeat_StartCheck(void);
void App_NegPrsHeat_SetWorkParams(void);
Heat_GetStatus_Reply_t *App_NegPrsHeat_GetStatus(void);
NPH_RunState_EnumDef App_NegPrsHeat_GetRunState(void);
void App_NegPrsHeat_ChangeState(NPH_RunState_EnumDef newState);

#ifdef __cplusplus
}
#endif
#endif  // APP_NEGPRSHEAT_H
/**************************End of file********************************/
