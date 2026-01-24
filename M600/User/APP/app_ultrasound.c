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
#include "drv_si5351.h"

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
    bool headConnected = (s_USCtrlInfo.probeStatus == E_IODEVICE_MODE_ULTRASOUND);
    
    // Combine connection state: bit[4]=head connection, bit[0]=foot switch
    // 0x00: Head connected, foot closed
    // 0x10: Head disconnected, foot closed
    // 0x01: Head connected, foot open
    // 0x11: Head disconnected, foot open
    if (headConnected && s_USCtrlInfo.FootSwitchStatus) {
        s_USCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_CONNECTED_FOOT_CLOSED;
    } else if (!headConnected && s_USCtrlInfo.FootSwitchStatus) {
        s_USCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_DISCONNECTED_FOOT_CLOSED;
    } else if (headConnected && !s_USCtrlInfo.FootSwitchStatus) {
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
            default:
                break;
        }
    }
}

void App_Ultrasound_Monitor(void)
{
    // Get the foot switch status and probe status
    s_USCtrlInfo.FootSwitchStatus = Drv_IODevice_GetFootSwitchState();

    // Get the probe status
    s_USCtrlInfo.probeStatus = Drv_IODevice_GetProbeStatus();
    if(s_USCtrlInfo.probeStatus != E_IODEVICE_MODE_ULTRASOUND)
    {
        App_Ultrasound_ChangeState(E_US_RUN_STOP);
        s_USCtrlInfo.ErrorCode = E_US_ERROR_PROBE_NOT_CONNECTED;
    }

}

void App_Ultrasound_SetFrequency(uint16_t frequency)
{
    if(frequency > 1400 || frequency < 1000)
    {
        LOG_E("Invalid frequency");
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        return;
    }
    // to be done ,set frequency to ultrasound
}

void App_UltraSound_SetLevel(uint8_t level)
{
    if(level > 6)
    {
        LOG_E("Invalid level");
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        return;
    }
    // to be done ,set level to ultrasound
}

void App_Ultrasound_SwitchWorkState(bool work_state)
{
    if(work_state == true)
    {
        // to be done ,switch work state to ultrasound

    } else {
        // to be done ,switch work state to ultrasound
    }
}

bool App_UltraSound_StartCheck()
{
    // 1. 检查下位机是否下发了发射超声指令
    if(s_USCtrlInfo.Trans.RxWorkState.work_state != 0x01) {
        LOG_E("Work state is not start");
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 2. 检查剩余工作时间是否大于0（0-3600s）
    if(s_USCtrlInfo.Trans.RxWorkState.work_time == 0 || s_USCtrlInfo.Trans.RxWorkState.work_time > 3600) {
        LOG_E("Invalid work time: %d", s_USCtrlInfo.Trans.RxWorkState.work_time);
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 3. 检查工作档位是否不等于0（0-20）
    if(s_USCtrlInfo.Trans.RxWorkState.work_level == 0 || s_USCtrlInfo.Trans.RxWorkState.work_level > 20) {
        LOG_E("Invalid work level: %d", s_USCtrlInfo.Trans.RxWorkState.work_level);
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 4. 检查脚踏开关是否闭合
    if(s_USCtrlInfo.FootSwitchStatus == false) {
        LOG_E("Foot switch is not closed");
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 5. 检查是否正确识别到超声治疗头
    if(s_USCtrlInfo.probeStatus != E_IODEVICE_MODE_ULTRASOUND) {
        LOG_E("Ultrasound probe not connected");
        s_USCtrlInfo.ErrorCode = E_US_ERROR_PROBE_NOT_CONNECTED;
        return false;
    }
    
    // 6. 检查是否有剩余可治疗次数
    if(s_USCtrlInfo.TreatTimes == 0) {
        LOG_E("No remaining treat times");
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 7. 检查配置参数是否有效
    if(s_USCtrlInfo.Trans.RxConfig.frequency == 0 || s_USCtrlInfo.Trans.RxConfig.temp_limit == 0 || s_USCtrlInfo.Trans.RxConfig.voltage == 0) {
        LOG_E("Invalid ultrasound config parameters: freq=%d, temp_limit=%d, voltage=%d", 
              s_USCtrlInfo.Trans.RxConfig.frequency, 
              s_USCtrlInfo.Trans.RxConfig.temp_limit, 
              s_USCtrlInfo.Trans.RxConfig.voltage);
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 8. 检查治疗参数是否有效
    if(s_USCtrlInfo.TreatParams.CurrentHigh == 0 || s_USCtrlInfo.TreatParams.CurrentLow == 0) {
        LOG_E("Invalid treatment parameters: CurrentHigh=%d, CurrentLow=%d", 
              s_USCtrlInfo.TreatParams.CurrentHigh, 
              s_USCtrlInfo.TreatParams.CurrentLow);
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 所有检查通过
    return true;
}

void App_UltraSound_SetWorkParams(void)
{
    // 设置工作参数
    s_USCtrlInfo.WorkLevel = s_USCtrlInfo.Trans.RxWorkState.work_level;
    s_USCtrlInfo.RemainTime = s_USCtrlInfo.Trans.RxWorkState.work_time;
    s_USCtrlInfo.Voltage = s_USCtrlInfo.Trans.RxConfig.voltage;
    s_USCtrlInfo.CurrentHigh = s_USCtrlInfo.TreatParams.CurrentHigh;
    s_USCtrlInfo.CurrentLow = s_USCtrlInfo.TreatParams.CurrentLow;
    s_USCtrlInfo.Frequency = s_USCtrlInfo.Trans.RxConfig.frequency;
    s_USCtrlInfo.TempLimit = s_USCtrlInfo.Trans.RxConfig.temp_limit;
    s_USCtrlInfo.TreatTimes = s_USCtrlInfo.TreatTimes - 1;
    //to be done ,save treat times to memory
    Drv_IODevice_ChangeChannel(CHANNEL_READY);
    Drv_SI5351_SetFrequency(s_USCtrlInfo.Frequency);
    Drv_SI5351_SetPulseWidthus(s_USCtrlInfo.WorkLevel);
}

bool App_UltraSound_IsCurrentNormal(void)
{
    bool isNormal = true;
    uint16_t current = Drv_ADC_GetRealValue(E_ADC_CHANNEL_US_I);
    if(current > s_USCtrlInfo.CurrentHigh)
    {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_CURRENT_TOO_HIGH;
        LOG_E("Current is too high: %d", current);
        isNormal = false;
    }
    else if(current < s_USCtrlInfo.CurrentLow)
    {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_CURRENT_TOO_LOW;
        LOG_E("Current is too low: %d", current);
        isNormal = false;
    }
    else
    {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_NONE;
    }
    return isNormal;

}

bool App_UltraSound_IsHeadTempNormal(void)
{
    bool isNormal = true;
    uint16_t temp = Drv_ADC_GetRealValue(E_ADC_CHANNEL_HAND_NTC);
    if(temp > s_USCtrlInfo.TempLimit)
    {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_TEMP_TOO_HIGH;
    }
    else if(temp < s_USCtrlInfo.TempLimit)
    {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_TEMP_TOO_LOW;
        LOG_E("Temp is too low: %d", temp);
        isNormal = false;
    }
    else
    {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_NONE;
    }
    return isNormal;
}


void App_Ultrasound_Process(void)
{
    // Process the ultrasound module
    App_UltraSound_UpdateStatus();
    App_UltraSound_RxDataHandle();
    App_Ultrasound_Monitor();

    // Handle the ultrasound state
    switch(s_USCtrlInfo.runState)
    {
        case E_US_RUN_INIT:
            if(App_Memory_LoadUSParams(&s_USCtrlInfo.TreatParams)) {
                s_USCtrlInfo.Trans.RxConfig.frequency = s_USCtrlInfo.TreatParams.Frequency;
                s_USCtrlInfo.Trans.RxConfig.temp_limit = s_USCtrlInfo.TreatParams.TempLimit;
                s_USCtrlInfo.Trans.RxConfig.voltage = s_USCtrlInfo.TreatParams.Voltage;
                s_USCtrlInfo.TreatTimes = s_USCtrlInfo.TreatParams.RemainTimes;
                
            } else {
                LOG_E("Failed to load ultrasound parameters");
                s_USCtrlInfo.ErrorCode = E_US_ERROR_READ_PARAMS_FAILED;
            }
            App_Ultrasound_ChangeState(E_US_RUN_IDLE);
            break;
        case E_US_RUN_IDLE:
            // 使用App_UltraSound_StartCheck进行启动前检查（包含所有参数检查）
            if(App_UltraSound_StartCheck()) {
                // 设置工作参数并启动超声发射
                App_UltraSound_SetWorkParams();

                Drv_IODevice_ChangeChannel(CHANNEL_US);
                App_Ultrasound_ChangeState(E_US_RUN_WORKING);
            }
            break;
        case E_US_RUN_WORKING:
            if(App_UltraSound_StartCheck() == false || 
            App_UltraSound_IsCurrentNormal() == false || 
            App_UltraSound_IsHeadTempNormal() == false ||
            s_USCtrlInfo.RemainTime == 0){
                App_Ultrasound_ChangeState(E_US_RUN_STOP);
            } else {
                s_USCtrlInfo.RemainTime -= TREAT_TASK_TIME;
            }
            break;
        case E_US_RUN_STOP:
            Drv_IODevice_ChangeChannel(CHANNEL_CLOSE);
            App_TreatMgr_ChangeState(E_TREATMGR_STATE_IDLE);
            break;
        default:
            break;
    }
}
/**************************End of file********************************/
