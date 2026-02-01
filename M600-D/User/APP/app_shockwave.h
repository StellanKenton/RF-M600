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
#include "app_treatmgr.h"

/* Work/Freq/Point limits */
#define SW_WORK_LEVEL_MAX          26          ///< Max work level (1-26)
#define SW_FREQ_LEVEL_MAX          16          ///< Max frequency level (1-16)
#define SW_WORK_POINT_MAX          10000       ///< Max work points per treatment
#define SW_VOLTAGE_THRESHOLD_MV    3000        ///< Min voltage threshold (3V = 3000mV)
#define SW_TEMP_MONITOR_PERIOD_MS  10          ///< Head temp monitor period (10ms)

/* PWM timing */
#define SW_PWM_ESW_P_HIGH_TIME_MS     5       ///< PWM_ESW+ high time (5ms)
#define SW_PWM_ESW_P_WAIT_TIME_MS     17      ///< Wait time after ESW_P before ESW_N (17ms)
#define SW_PWM_ESW_N_BASE_TIME_MS     3       ///< PWM_ESW-N base high time (3ms)
#define SW_PWM_ESW_N_STEP_TIME_MS     0.28f   ///< PWM_ESW-N step per level (0.28ms)

typedef enum
{
    E_SW_RUN_INIT = 0,
    E_SW_RUN_IDLE,
    E_SW_RUN_WORKING,
    E_SW_RUN_STOP,
    E_SW_RUN_WAIT_RETURN,
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
    Treat_Times_EnumDef TreatCountsState;
    SW_PWM_State_EnumDef pwmState;
    bool isWaitReturn;

    uint16_t TempLimit;            ///< Head temp limit (0.1 C)
    uint16_t TreatCounts;          ///< Remaining treatment times
    uint16_t CurrentHigh_ESW_P;    ///< PWM_ESW+ current high threshold (mV)
    uint16_t CurrentLow_ESW_P;     ///< PWM_ESW+ current low threshold (mV)
    uint16_t CurrentHigh_ESW_N;   ///< PWM_ESW-N current high threshold (mV)
    uint16_t CurrentLow_ESW_N;    ///< PWM_ESW-N current low threshold (mV)
    
    uint8_t WorkLevel;             ///< Work level (1-26)
    uint8_t FreqLevel;             ///< Frequency level (1-16)
    uint16_t RemainPoints;         ///< Remaining work points
    uint16_t HeadTemp;             ///< Head temperature (0.1 C)
    
    uint8_t LastStartState;
    uint8_t ErrorCode;
    
    SW_TreatParams_t TreatParams;
    SW_TransData_t Trans;
    
    /* PWM timing (ms) */
    uint32_t pwmStateStartTime;    ///< Current PWM state start time (ms)
    uint32_t cycleStartTime;       ///< Current cycle start time (ms)
    uint32_t cyclePeriodMs;        ///< Cycle period (ms), 1000/freqLevel
    uint32_t pwmESW_NHighTimeMs;   ///< PWM_ESW-N high time (ms)
    
    /* Monitoring */
    uint32_t lastTempMonitorTime;  ///< Last head temp monitor tick
} SW_CtrlInfo_t;

void App_Shockwave_Init(void);
void App_Shockwave_Process(void);
bool App_Shockwave_StartCheck(void);
void App_Shockwave_SetWorkParams(void);
SW_GetStatus_Reply_t *App_Shockwave_GetStatus(void);
SW_RunState_EnumDef App_Shockwave_GetRunState(void);
void App_Shockwave_ChangeState(SW_RunState_EnumDef newState);
#ifdef __cplusplus
}
#endif
#endif  // APP_SHOCKWAVE_H
/**************************End of file********************************/
