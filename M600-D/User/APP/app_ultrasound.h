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
#include "app_treatmgr.h"

#define PULSE_REPEAT_TIME_BASE_MS    20     
#define PULSE_REPEAT_TIME_STEP_MS    0.5f   
#define PULSE_REPEAT_TIME_MIN_MS     0.5f    
#define PULSE_REPEAT_TIME_MAX_MS     20     
#define WORK_LEVEL_MAX               40      

#define VOLTAGE_ADJUST_LIMIT_MV       2000    

typedef enum
{
    E_US_RUN_INIT = 0,
    E_US_RUN_IDLE,
    E_US_RUN_WORKING,
    E_US_RUN_STOP,
    E_US_RUN_WAIT_RETURN,
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
    Treat_Times_EnumDef TreatCountsState;
    bool isWaitReturn;
    uint16_t Voltage;              ///< ???? (mV)
    uint16_t VoltageBase;          ///< ???????? (mV)??????????
    uint16_t CurrentHigh;
    uint16_t CurrentLow;
    uint16_t Frequency;
    uint16_t TempLimit;
    uint32_t TreatCounts;  

    uint8_t WorkLevel;
    uint16_t HeadTemp;
    uint16_t TreatRemainTimes;
    
    uint8_t ErrorCode;
    uint8_t StartCheckStep;
    
    US_TreatParams_t TreatParams;
    UltraSound_TransData_t Trans;
} US_CtrlInfo_t;


void App_Ultrasound_Init(void);
void App_Ultrasound_Process(void);
bool App_UltraSound_StartCheck(void);
void App_UltraSound_SetWorkParams(void);
US_RunState_EnumDef App_Ultrasound_GetRunState(void);
void App_Ultrasound_ChangeState(US_RunState_EnumDef newState);
US_GetStatus_Reply_t *App_UltraSound_GetStatus(void);
US_SetConfig_Reply_t *App_UltraSound_GetConfig(void);
#ifdef __cplusplus
}
#endif
#endif  // APP_ULTRASOUND_H
/**************************End of file********************************/
