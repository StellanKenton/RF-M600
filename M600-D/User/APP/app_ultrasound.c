/***********************************************************************************
* @file     : app_ultrasound.c
* @brief    : Ultrasound treatment module implementation
* @details  : State machine, work time, current/temp monitor, frequency and DAC control
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
#include "drv_dac.h"
#include "drv_adc.h"
#include "log.h"
#include "drv_si5351.h"
#include "drv_delay.h"
#include "bsp_tim.h"

static US_CtrlInfo_t s_USCtrlInfo;

void App_UltraSound_UpdateStatus(void)
{
    // Update work state
    if(s_USCtrlInfo.runState == E_US_RUN_WORKING) {
        s_USCtrlInfo.Trans.TxStatus.work_state = 0x01;
    }else{
        s_USCtrlInfo.Trans.TxStatus.work_state = 0x00;
    }
    s_USCtrlInfo.Trans.TxStatus.frequency = s_USCtrlInfo.Frequency;
    s_USCtrlInfo.Trans.TxStatus.temp_limit = s_USCtrlInfo.TempLimit;
    s_USCtrlInfo.Trans.TxStatus.remain_time = s_USCtrlInfo.TreatCounts/1000;
    s_USCtrlInfo.Trans.TxStatus.work_level = s_USCtrlInfo.WorkLevel;
    s_USCtrlInfo.Trans.TxStatus.head_temp = s_USCtrlInfo.HeadTemp;
    s_USCtrlInfo.Trans.TxStatus.remain_treatment_count = s_USCtrlInfo.TreatRemainTimes;
    // Probe and foot switch status refreshed by mgr; conn_state uploaded by comm based on mgr state; only protocol fields kept here
    bool headConnected = (App_TreatMgr_GetProbeStatus() == E_IODEVICE_MODE_ULTRASOUND);
    bool footClosed = App_TreatMgr_GetFootSwitchClosed();
    if (headConnected && footClosed) {
        s_USCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_CONNECTED_FOOT_CLOSED;
    } else if (!headConnected && footClosed) {
        s_USCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_DISCONNECTED_FOOT_CLOSED;
    } else if (headConnected && !footClosed) {
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

    if(pTransData->flag.bits.Sync_Config)
    {
        const US_TreatParams_t *pParams = App_Memory_GetUSParams();

        s_USCtrlInfo.TreatParams = *pParams;
        if(s_USCtrlInfo.runState != E_US_RUN_WORKING)
        {
            s_USCtrlInfo.Frequency = pParams->Frequency;
            s_USCtrlInfo.TempLimit = pParams->TempLimit;
            s_USCtrlInfo.TreatRemainTimes = pParams->TreatRemainTimes;
            s_USCtrlInfo.CurrentHigh = pParams->CurrentHigh;
            s_USCtrlInfo.CurrentLow = pParams->CurrentLow;
            s_USCtrlInfo.VoltageBase = pParams->Voltage;
        }

        pTransData->flag.bits.Sync_Config = 0;
        LOG_I("US config synced: freq=%d, temp=%d, voltage=%d, remain=%d, runtime_updated=%d",
              pParams->Frequency, pParams->TempLimit, pParams->Voltage,
              pParams->TreatRemainTimes, s_USCtrlInfo.runState != E_US_RUN_WORKING);
    }
}

void App_Ultrasound_ChangeState(US_RunState_EnumDef newState)
{
    if(newState != s_USCtrlInfo.runState && newState < E_US_RUN_MAX)
    {
        US_RunState_EnumDef oldState = s_USCtrlInfo.runState;
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
                // Buzzer beep when starting work (2s)
                Drv_IODevice_StartBuzzer(2000);
                break;
            case E_US_RUN_STOP:
                LOG_I("Ultrasound state changed to STOP");
                // Only beep when output really stops, not when a module switch forces STOP from IDLE.
                if(oldState == E_US_RUN_WORKING) {
                    Drv_IODevice_StartBuzzer(2000);
                }
                break;
            default:
                break;
        }
    }
}

void App_Ultrasound_WorkTimeHandle(void)
{
    static uint8_t s_lastRxWorkState = 0x00;
    if(s_USCtrlInfo.Trans.RxWorkState.work_state != s_lastRxWorkState) {
        /* When work_state changes from other state to 0x02(Reset), enter E_TREAT_TIMES_RESET */
        if (s_USCtrlInfo.Trans.RxWorkState.work_state == WORK_STATE_RESET && s_lastRxWorkState != WORK_STATE_RESET)
        {
            s_USCtrlInfo.TreatCountsState = E_TREAT_TIMES_RESET;
        }
		s_lastRxWorkState = s_USCtrlInfo.Trans.RxWorkState.work_state;
    }

    switch(s_USCtrlInfo.TreatCountsState)
    {
        case E_TREAT_TIMES_POWER_ON:
            if(s_USCtrlInfo.Trans.RxWorkState.work_time > 0 && s_USCtrlInfo.TreatRemainTimes > 0)
            {
                s_USCtrlInfo.TreatCounts = s_USCtrlInfo.Trans.RxWorkState.work_time * 1000;
                s_USCtrlInfo.TreatCountsState = E_TREAT_TIMES_WORKING;
                s_USCtrlInfo.TreatParams.TreatRemainTimes = s_USCtrlInfo.TreatRemainTimes-1;
                s_USCtrlInfo.TreatRemainTimes--;
                App_Memory_SaveUSParams(&s_USCtrlInfo.TreatParams);
                LOG_I("Remaining treat times decreased to: %d", s_USCtrlInfo.TreatRemainTimes);
            }
            break;
        case E_TREAT_TIMES_RESET:
            if(s_USCtrlInfo.Trans.RxWorkState.work_time > 0 && s_USCtrlInfo.TreatRemainTimes > 0)
            {
                s_USCtrlInfo.TreatCounts = s_USCtrlInfo.Trans.RxWorkState.work_time * 1000;
                s_USCtrlInfo.TreatCountsState = E_TREAT_TIMES_WORKING;
                s_USCtrlInfo.TreatParams.TreatRemainTimes = s_USCtrlInfo.TreatRemainTimes-1;
                s_USCtrlInfo.TreatRemainTimes--;
                App_Memory_SaveUSParams(&s_USCtrlInfo.TreatParams);
                LOG_I("Remaining treat times decreased to: %d", s_USCtrlInfo.TreatRemainTimes);
            }
            break;
        case E_TREAT_TIMES_WORKING:
            if(s_USCtrlInfo.TreatCounts > 0 && s_USCtrlInfo.runState == E_US_RUN_WORKING)
            {
                s_USCtrlInfo.TreatCounts-= TREAT_TASK_TIME;
                if(s_USCtrlInfo.TreatCounts < TREAT_TASK_TIME)
                {
                    s_USCtrlInfo.TreatCounts = 0;
                }
            }
            if(s_USCtrlInfo.TreatCounts == 0)
            {
                s_USCtrlInfo.TreatCountsState = E_TREAT_TIMES_WAIT;
            }
            break;
		case E_TREAT_TIMES_WAIT:
			break;
        default:
            break;
    }

}

void App_Ultrasound_SetFrequency(uint16_t frequency)
{
    if(frequency > 1400 || frequency < 700)
    {
        LOG_E("Invalid frequency: %d (range: 700-1400kHz)", frequency);
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        return;
    }
    // Set frequency to SI5351
    frequency = Drv_SI5351_SetFrequency(frequency);
    s_USCtrlInfo.Frequency = frequency;
    LOG_I("Ultrasound frequency set to: %d kHz", frequency);
}


bool App_UltraSound_StartCheck()
{
    // 1. Check if host has sent ultrasound transmit command
    if(s_USCtrlInfo.Trans.RxWorkState.work_state != 0x01) {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        s_USCtrlInfo.StartCheckStep = 1;
        return false;
    }

    // 2. Check if remaining work time is > 0 (range 0-3600s)
    if(s_USCtrlInfo.Trans.RxWorkState.work_time == 0 || s_USCtrlInfo.Trans.RxWorkState.work_time > 3600) {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        s_USCtrlInfo.StartCheckStep = 2;
        return false;
    }

    // 3. Check if work level is valid (range 1-40)
    if(s_USCtrlInfo.Trans.RxWorkState.work_level == 0 || s_USCtrlInfo.Trans.RxWorkState.work_level > WORK_LEVEL_MAX) {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        s_USCtrlInfo.StartCheckStep = 3;
        return false;
    }

    // 4. Check if foot switch is closed
    if(App_TreatMgr_GetFootSwitchClosed()) {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        s_USCtrlInfo.StartCheckStep = 4;
        return false;
    }

    // 5. Check if ultrasound probe is correctly identified
    if(App_TreatMgr_GetProbeStatus() != E_IODEVICE_MODE_ULTRASOUND) {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_PROBE_NOT_CONNECTED;
        s_USCtrlInfo.StartCheckStep = 5;
        return false;
    }

    // 6. Check if there are remaining treatment times
    if(s_USCtrlInfo.TreatCounts == 0) {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        s_USCtrlInfo.StartCheckStep = 6;
        return false;
    }

    // 7. Check if config parameters are valid
    if(s_USCtrlInfo.Trans.RxConfig.frequency == 0 || s_USCtrlInfo.Trans.RxConfig.temp_limit == 0 || s_USCtrlInfo.Trans.RxConfig.voltage == 0) {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        s_USCtrlInfo.StartCheckStep = 7;
        return false;
    }

    // 8. Check if treatment parameters are valid
    if(s_USCtrlInfo.CurrentHigh == 0) {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        s_USCtrlInfo.StartCheckStep = 8;
        return false;
    }

    // All checks passed
    return true;
}

void App_UltraSound_SetWorkParams(void)
{
    // Set work parameters
    s_USCtrlInfo.WorkLevel = s_USCtrlInfo.Trans.RxWorkState.work_level;
    s_USCtrlInfo.Voltage = s_USCtrlInfo.Trans.RxConfig.voltage;
    s_USCtrlInfo.VoltageBase = s_USCtrlInfo.Trans.RxConfig.voltage;  // Save base voltage for over-limit check
    s_USCtrlInfo.Frequency = s_USCtrlInfo.Trans.RxConfig.frequency;
    s_USCtrlInfo.TempLimit = s_USCtrlInfo.Trans.RxConfig.temp_limit;

    // Configure work voltage and frequency
    App_Ultrasound_SetFrequency(s_USCtrlInfo.Frequency);
    // Set initial work voltage
    Drv_DAC_SetVoltage(s_USCtrlInfo.Voltage);
    // delay 100ms
    Drv_Delay_ms(100);
}

bool App_UltraSound_IsCurrentNormal(void)
{
    bool isNormal = true;
    uint16_t current = Drv_ADC_GetRealValue(BSP_ADC_CH_US_I);
    uint16_t currentVoltage = Drv_DAC_GetVoltage();
    int16_t voltageAdjust = 0;
    uint16_t newVoltage = currentVoltage;

    if(TreatGetRunFlag()) {
        return true;
    }

    if(current > s_USCtrlInfo.CurrentHigh)
    {
        // Current too high, need to reduce voltage
        // Simple PI control: adjust voltage based on current error
        int16_t currentError = current - ((s_USCtrlInfo.CurrentHigh + s_USCtrlInfo.CurrentLow) / 2);
        voltageAdjust = -(currentError * 10) / 100;  // Simple proportional control

        s_USCtrlInfo.ErrorCode = E_US_ERROR_CURRENT_TOO_HIGH;
        LOG_W("Current is too high: %d (target: %d-%d)", current, s_USCtrlInfo.CurrentLow, s_USCtrlInfo.CurrentHigh);
    }
    else if(current < s_USCtrlInfo.CurrentLow)
    {
        // Current too low, need to increase voltage
        int16_t currentError = ((s_USCtrlInfo.CurrentHigh + s_USCtrlInfo.CurrentLow) / 2) - current;
        voltageAdjust = (currentError * 10) / 100;  // Simple proportional control

        s_USCtrlInfo.ErrorCode = E_US_ERROR_CURRENT_TOO_LOW;
        LOG_W("Current is too low: %d (target: %d-%d)", current, s_USCtrlInfo.CurrentLow, s_USCtrlInfo.CurrentHigh);
    }
    else
    {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_NONE;
    }

    // If voltage adjustment is needed
    if(voltageAdjust != 0)
    {
        newVoltage = currentVoltage + voltageAdjust;

        // Check if voltage adjustment exceeds limit (+/-2V)
        // Limit voltage range
        if(newVoltage > 2000)
        {
            newVoltage = 2000;
        }
        else if(newVoltage <= 1000)
        {
            newVoltage = 1000;
        }

        // Set new voltage
        Drv_DAC_SetVoltage(newVoltage);
        LOG_I("Voltage adjusted: %d -> %d mV (current: %d)", currentVoltage, newVoltage, current);
    }

    if(currentVoltage > s_USCtrlInfo.VoltageBase + VOLTAGE_ADJUST_LIMIT_MV || currentVoltage < s_USCtrlInfo.VoltageBase - VOLTAGE_ADJUST_LIMIT_MV)
    {
        // Voltage over limit, report error
        s_USCtrlInfo.ErrorCode = E_US_ERROR_VOLTAGE_OVER_LIMIT;
        LOG_E("Voltage adjust over limit: %d mV (base: %d mV, limit: ±%d mV)",
              newVoltage, s_USCtrlInfo.VoltageBase, VOLTAGE_ADJUST_LIMIT_MV);
        isNormal = false;
    }
    return isNormal;
}

bool App_UltraSound_IsHeadTempNormal(void)
{
    bool isNormal = true;
    uint16_t temp = Drv_ADC_GetRealValue(BSP_ADC_CH_HAND_NTC);
    s_USCtrlInfo.HeadTemp = temp;

    if(TreatGetRunFlag()) {
        return true;
    }

    if(temp > s_USCtrlInfo.TempLimit)
    {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_TEMP_TOO_HIGH;
        // Temp over limit, auto reduce level (min 0)
        if(s_USCtrlInfo.WorkLevel > 0)
        {
            LOG_W("Head temperature too high: %d (limit: %d)", temp, s_USCtrlInfo.TempLimit);
            s_USCtrlInfo.WorkLevel--;
            LOG_W("Auto reduce level to: %d", s_USCtrlInfo.WorkLevel);
        }
    }
    else
    {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_NONE;
    }
    return isNormal;
}

void App_Ultrasound_CheckProbe(void)
{
    if(App_TreatMgr_GetProbeStatus() != E_IODEVICE_MODE_ULTRASOUND) {
        s_USCtrlInfo.isWaitReturn = true;
    }

    if(s_USCtrlInfo.isWaitReturn) {
        App_Ultrasound_ChangeState(E_US_RUN_STOP);
    }
}

void App_Ultra_RunChangeLevel()
{
    if(s_USCtrlInfo.WorkLevel != s_USCtrlInfo.Trans.RxWorkState.work_level) 
    {
        if(s_USCtrlInfo.Trans.RxWorkState.work_level <= 40) {
            s_USCtrlInfo.WorkLevel = s_USCtrlInfo.Trans.RxWorkState.work_level;
        }
    }
}


void App_Ultrasound_Process(void)
{
    // Process the ultrasound module
    App_UltraSound_UpdateStatus();
    App_UltraSound_RxDataHandle();
    App_Ultrasound_WorkTimeHandle();
    App_Ultrasound_CheckProbe();
    // Handle the ultrasound state
    switch(s_USCtrlInfo.runState)
    {
        case E_US_RUN_INIT:
            {
                const US_TreatParams_t *pParams = App_Memory_GetUSParams();
                s_USCtrlInfo.TreatParams = *pParams;
                s_USCtrlInfo.Trans.RxConfig.frequency = pParams->Frequency;
                s_USCtrlInfo.Trans.RxConfig.temp_limit = pParams->TempLimit;
                s_USCtrlInfo.Trans.RxConfig.voltage = pParams->Voltage;
                s_USCtrlInfo.TreatRemainTimes = pParams->TreatRemainTimes;
                s_USCtrlInfo.CurrentHigh = pParams->CurrentHigh;
                s_USCtrlInfo.CurrentLow = pParams->CurrentLow;
                s_USCtrlInfo.VoltageBase = pParams->Voltage;
            }
			// Switch relay pwr_control1 to ultrasound channel
            Drv_IODevice_ChangeChannel(CHANNEL_US);
            App_Ultrasound_ChangeState(E_US_RUN_IDLE);
            break;
        case E_US_RUN_IDLE:
            // Use App_UltraSound_StartCheck for pre-start check (all param checks)
            if(App_UltraSound_StartCheck()) {
                // Set work params and start ultrasound transmit
                App_UltraSound_SetWorkParams();
                Drv_SI5351_SetComplementaryPWM(true);
                // pwr_control2 switch to output enabled (normally disabled)
                Drv_IODevice_ChangeChannel(CHANNEL_RF_US_READY);
                App_Ultrasound_ChangeState(E_US_RUN_WORKING);
            }
            break;
        case E_US_RUN_WORKING:
            // Check all conditions
            if(App_UltraSound_StartCheck() == false ||
            App_UltraSound_IsCurrentNormal() == false ||
            App_UltraSound_IsHeadTempNormal() == false ){
              App_Ultrasound_ChangeState(E_US_RUN_STOP);
            }
			App_Ultra_RunChangeLevel();
            break;
        case E_US_RUN_STOP:
			s_USCtrlInfo.WorkLevel = 0;
			s_USCtrlInfo.Trans.RxWorkState.work_state = 0;
            // Close output channel
            Drv_IODevice_ChangeChannel(CHANNEL_RF_US_CLOSE);
            App_Ultrasound_ChangeState(E_US_RUN_IDLE);
            if(s_USCtrlInfo.isWaitReturn) {
                App_Ultrasound_ChangeState(E_US_RUN_WAIT_RETURN);
                LOG_I("US: Wait return");
                s_USCtrlInfo.isWaitReturn = false;
            }
            break;
        case E_US_RUN_WAIT_RETURN:
            break;
        default:
            break;
    }
}
/**
 * @brief Initialize ultrasound module
 */
void App_Ultrasound_Init(void)
{
    // Initialize control info structure
    memset(&s_USCtrlInfo, 0, sizeof(US_CtrlInfo_t));

    // Set initial state
    s_USCtrlInfo.runState = E_US_RUN_INIT;
    s_USCtrlInfo.ErrorCode = E_US_ERROR_NONE;
    s_USCtrlInfo.WorkLevel = 0;
    s_USCtrlInfo.TreatCounts = 0;

    LOG_I("Ultrasound module initialized");
}

US_GetStatus_Reply_t *App_UltraSound_GetStatus(void)
{
    return &s_USCtrlInfo.Trans.TxStatus;
}
US_SetConfig_Reply_t *App_UltraSound_GetConfig(void)
{
    return &s_USCtrlInfo.Trans.TxConfig;
}

US_RunState_EnumDef App_Ultrasound_GetRunState(void)
{
    return s_USCtrlInfo.runState;
}

void App_Ultrasound_SetHighFreqPowerHandle10us(void)
{
    static uint32_t s_highFreqPowerWorkTimeUs = 0U;
    static uint32_t s_activeWindowUs = 0U;
    static uint8_t s_lastWorkLevel = 0xFFU;
    static bool s_outputEnabled = false;
    bool enableOutput;

    if(s_USCtrlInfo.runState != E_US_RUN_WORKING) {
        s_highFreqPowerWorkTimeUs = 0U;
        s_activeWindowUs = 0U;
        s_lastWorkLevel = 0xFFU;
        if(s_outputEnabled) {
            Drv_IO_HighFreqPowerOutput(false);
            s_outputEnabled = false;
        }
        return;
    }

    if(s_USCtrlInfo.WorkLevel != s_lastWorkLevel) {
        s_lastWorkLevel = s_USCtrlInfo.WorkLevel;
        s_activeWindowUs = (uint32_t)s_lastWorkLevel * 500U;
    }

    if(s_highFreqPowerWorkTimeUs >= 20000U) {
        s_highFreqPowerWorkTimeUs = 0U;
    }

    enableOutput = (s_highFreqPowerWorkTimeUs < s_activeWindowUs);
    if(enableOutput != s_outputEnabled) {
        Drv_IO_HighFreqPowerOutput(enableOutput);
        s_outputEnabled = enableOutput;
    }

    s_highFreqPowerWorkTimeUs += 10U;
}

/**************************End of file********************************/
