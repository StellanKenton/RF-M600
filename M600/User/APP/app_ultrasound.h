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

typedef enum
{
    E_US_RUN_WAIT = 0,
    E_US_RUN_INIT,
    E_US_RUN_IDLE,
    E_US_RUN_WORKING,
    E_US_RUN_STOP,
    E_US_RUN_ERROR,
    E_US_RUN_MAX,
} US_RunState_EnumDef;


typedef struct
{
    US_RunState_EnumDef runState;
    uint16_t Voltage;
    uint16_t Current;
    uint16_t Frequency;
    uint16_t TempLimit;
    uint16_t RemainTime;
    uint16_t WorkTime;
    uint8_t WorkLevel;
    uint16_t HeadTemp;
    uint8_t ConnState;
    uint8_t ErrorCode;
    uint16_t RemainTreatTime;
} US_CtrlInfo_t;


void App_Ultrasound_Init(void);
void App_Ultrasound_Process(void);


#ifdef __cplusplus
}
#endif
#endif  // APP_ULTRASOUND_H
/**************************End of file********************************/
