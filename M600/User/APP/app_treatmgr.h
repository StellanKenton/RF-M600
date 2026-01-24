/************************************************************************************
* @file     : app_treatmgr.h
* @brief    : 
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef APP_TREATMGR_H
#define APP_TREATMGR_H

#include <string.h>
#include <stdbool.h>
#include "stdint.h"

#ifdef __cplusplus
#include <iostream>
extern "C" {
#endif

#include "drv_iodevice.h"

#define   TREAT_TASK_TIME     10       // 10ms


typedef enum
{
    E_TREATMGR_STATE_IDLE = 0,
    E_TREATMGR_STATE_RADIO_FREQUENCY,
    E_TREATMGR_STATE_SHOCK_WAVE,
    E_TREATMGR_STATE_NEGATIVE_PRESSURE_HEAT,
    E_TREATMGR_STATE_ULTRASOUND,   
    E_TREATMGR_STATE_ERROR,
    E_TREATMGR_STATE_MAX,
} TreatMgr_State_EnumDef;

typedef struct
{
    TreatMgr_State_EnumDef eState;
    TreatMgr_State_EnumDef preState;
    IODevice_WorkingMode_EnumDef eProbeStatus;
    IODevice_WorkingMode_EnumDef preProbeStaus;
} TreatMgr_t;


void App_TreatMgr_Init(void);
void App_TreatMgr_Process(void);
void App_TreatMgr_ChangeState(TreatMgr_State_EnumDef newState);

#ifdef __cplusplus
}
#endif
#endif  // APP_TREATMGR_H
/**************************End of file********************************/
