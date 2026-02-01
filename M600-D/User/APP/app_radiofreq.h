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
#include "app_treatmgr.h"

/* ??????????????1MHz */
#define RF_FREQUENCY_KHZ           1000        ///< ?????????? (kHz)
#define RF_WORK_LEVEL_MAX          20          ///< ????? (0-20)
#define RF_VOLTAGE_MIN_MV          11000       ///< ??????? (11V = 11000mV)
#define RF_VOLTAGE_MAX_MV          30000       ///< ??????? (30V = 30000mV)
#define RF_VOLTAGE_INIT_MV         7000        ///< ????????? (7V = 7000mV)
#define RF_CURRENT_THRESHOLD_MV    500         ///< ????? (0.5V = 500mV)
#define RF_CURRENT_MONITOR_PERIOD_MS   10      ///< ?????? (10ms)
#define RF_TEMP_MONITOR_PERIOD_MS      1000    ///< ?????? (1s)

/* ??????????1-20????11-30V */
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
    Treat_Times_EnumDef TreatCountsState;
    bool isWaitReturn;
    uint16_t Voltage;              
    uint16_t VoltageTarget;        
    uint16_t CurrentHigh;        
    uint16_t CurrentLow;          
    uint16_t TempLimit;            
    uint16_t TreatCounts;  
    uint8_t LastStartState;
    
    uint8_t WorkLevel;             
    uint16_t HeadTemp;             
    uint16_t TreatRemainTimes;           
    
    uint8_t ErrorCode;
    
    RF_TreatParams_t TreatParams;
    RF_TransData_t Trans;
    
    /* ?????? */
    uint32_t lastCurrentMonitorTime;   ///< ???????????
    uint32_t lastTempMonitorTime;      ///< ???????????
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
