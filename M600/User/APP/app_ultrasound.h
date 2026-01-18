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

typedef enum
{
    E_US_RUN_INIT = 0,
    E_US_RUN_IDLE,
    E_US_RUN_WORKING,
    E_US_RUN_STOP,
    E_US_RUN_MAX,
} US_RunState_EnumDef;


typdef enum {
    E_US_ERROR_NONE = 0,
    E_US_ERROR_PROBE_NOT_CONNECTED,
    E_US_ERROR_READ_PARAMS_FAILED,
    E_US_ERROR_INVALID_PARAMS,
    E_US_ERROR_MAX,
}Ultrasound_ErrorCode_EnumDef;

typedef struct
{
    US_RunState_EnumDef runState;
    uint16_t Voltage;
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


#ifdef __cplusplus
}
#endif
#endif  // APP_ULTRASOUND_H
/**************************End of file********************************/
