/***********************************************************************************
* @file     : app_ultrasound.c
* @brief    : 
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "app_ultrasound.h"
#include "app_treatmgr.h"
#include "app_memory.h"
#include "app_comm.h"
#include "drv_iodevice.h"
#include "log.h"

static US_CtrlInfo_t s_USCtrlInfo;


void App_UltraSound_UpdateStatus(void)
{
    // Update work state
    if(s_USCtrlInfo.runState == E_US_RUN_WORKING) {
        s_USCtrlInfo.Trans.TxStatus.work_state = 0x01;
    } else if(s_USCtrlInfo.runState == E_US_RUN_STOP) {
        s_USCtrlInfo.Trans.TxStatus.work_state = 0x00;
    }
    s_USCtrlInfo.Trans.TxStatus.frequency = s_USCtrlInfo.Frequency;
    s_USCtrlInfo.Trans.TxStatus.temp_limit = s_USCtrlInfo.TempLimit;
    s_USCtrlInfo.Trans.TxStatus.remain_time = s_USCtrlInfo.RemainTime;
    s_USCtrlInfo.Trans.TxStatus.work_level = s_USCtrlInfo.WorkLevel;
    s_USCtrlInfo.Trans.TxStatus.head_temp = s_USCtrlInfo.HeadTemp;
    // Get probe connection status and foot switch state
    IODevice_WorkingMode_EnumDef probeStatus = Drv_IODevice_GetProbeStatus();
    bool footSwitchClosed = Drv_IODevice_GetFootSwitchState();
    bool headConnected = (probeStatus == E_IODEVICE_MODE_ULTRASOUND);
    
    // Combine connection state: bit[4]=head connection, bit[0]=foot switch
    // 0x00: Head connected, foot closed
    // 0x10: Head disconnected, foot closed
    // 0x01: Head connected, foot open
    // 0x11: Head disconnected, foot open
    if (headConnected && footSwitchClosed) {
        s_USCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_CONNECTED_FOOT_CLOSED;
    } else if (!headConnected && footSwitchClosed) {
        s_USCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_DISCONNECTED_FOOT_CLOSED;
    } else if (headConnected && !footSwitchClosed) {
        s_USCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_CONNECTED_FOOT_OPEN;
    } else {
        s_USCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_DISCONNECTED_FOOT_OPEN;
    }
    s_USCtrlInfo.Trans.TxStatus.error_code = s_USCtrlInfo.ErrorCode;
}


void App_UltraSound_RxDataHandle(void)
{
    UltraSound_TransData_t *pTransData = App_Comm_GetUSTransData();
    if(pTransData->RxValidFlag[PROTOCOL_CMD_SET_WORK_STATE])
    {
        s_USCtrlInfo.Trans.RxWorkState = pTransData->RxWorkState;
    }
    if(pTransData->RxValidFlag[PROTOCOL_CMD_SET_CONFIG])
    {
        s_USCtrlInfo.Trans.RxConfig = pTransData->RxConfig;
    }
}

void App_Ultrasound_ChangeState(US_RunState_EnumDef newState)
{
    if(newState != s_USCtrlInfo.runState && newState < E_US_RUN_MAX)
    {
        s_USCtrlInfo.runState = newState;
        switch(newState)
        {
            case E_US_RUN_INIT:
                LOG_I("Ultrasound state changed to INIT");
                break;
            case E_US_RUN_IDLE:
                LOG_I("Ultrasound state changed to IDLE");
                break;
            case E_US_RUN_WORKING:
                LOG_I("Ultrasound state changed to WORKING");
                break;
            case E_US_RUN_STOP:
                LOG_I("Ultrasound state changed to STOP");
                break;
            case E_US_RUN_ERROR:
                LOG_I("Ultrasound state changed to ERROR");
                break;
            default:
                break;
        }
    }
}

void App_Ultrasound_Monitor(void)
{
    s_USCtrlInfo.FootSwitchStatus = Drv_IODevice_GetFootSwitchState();
}


void App_Ultrasound_Process(void)
{
    // Process the ultrasound module
    App_UltraSound_UpdateStatus();
    App_UltraSound_RxDataHandle();

    // Handle the ultrasound state
    switch(s_USCtrlInfo.runState)
    {
        case E_US_RUN_INIT:
            if(App_Memory_LoadUSParams(&s_USCtrlInfo.TreatParams)) {
                s_USCtrlInfo.Frequency = s_USCtrlInfo.TreatParams.Frequency;
                s_USCtrlInfo.TempLimit = s_USCtrlInfo.TreatParams.TempLimit;
                s_USCtrlInfo.TreatTimes = s_USCtrlInfo.TreatParams.RemainTimes;
                
            } else {
                LOG_E("Failed to load ultrasound parameters");
                s_USCtrlInfo.Frequency = 1200;
                s_USCtrlInfo.TempLimit = 400;
                s_USCtrlInfo.TreatTimes = 0;
            }
            App_Ultrasound_ChangeState(E_US_RUN_IDLE);
            break;
        case E_US_RUN_IDLE:
            break;
        case E_US_RUN_WORKING:
            break;
        case E_US_RUN_STOP:
            App_TreatMgr_ChangeState(E_TREATMGR_STATE_IDLE);
            break;
        case E_US_RUN_ERROR:
            break;
        default:
            break;
    }
}
/**************************End of file********************************/
