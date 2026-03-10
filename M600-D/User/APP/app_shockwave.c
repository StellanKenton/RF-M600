/***********************************************************************************
* @file     : app_shockwave.c
* @brief    : Shock Wave treatment module implementation
* @details  :
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2025
**********************************************************************************/
#include "app_shockwave.h"
#include "app_treatmgr.h"
#include "app_memory.h"
#include "app_comm.h"
#include "drv_iodevice.h"
#include "drv_adc.h"
#include "drv_tim.h"
#include "drv_delay.h"
#include "log.h"
#include <string.h>

static SW_CtrlInfo_t s_SWCtrlInfo;

/**
 * @brief Calculate cycle period from frequency level (1-16; level1=1000ms, level16=62.5ms)
 * @param freqLevel Frequency level (1-16)
 * @retval Cycle period in milliseconds
 */
static uint32_t App_Shockwave_CalculateCyclePeriod(uint8_t freqLevel)
{
    if(freqLevel == 0 || freqLevel > SW_FREQ_LEVEL_MAX) {
        freqLevel = 1;
    }
    // period_ms = 1000 / freqLevel
    // level 1: 1000ms, level 16: 62.5ms
    return 1000 / freqLevel;
}

/**
 * @brief Calculate PWM_ESW-N high time from work level
 * @param level Work level (1-26)
 * @retval High time in milliseconds
 */
static uint32_t App_Shockwave_CalculateESW_NHighTime(uint8_t level)
{
    if(level == 0) {
        return 0;
    }
    if(level > SW_WORK_LEVEL_MAX) {
        level = SW_WORK_LEVEL_MAX;
    }
    // high_time_us = 3000 + (level - 1) * 280 (i.e. 3 + (level-1)*0.28 ms)
    uint32_t time_us = 3000 + (level - 1) * 280;
    return (time_us + 500) / 1000;
}

void App_Shockwave_UpdateStatus(void)
{
    // Update work state
    if(s_SWCtrlInfo.runState == E_SW_RUN_WORKING) {
        s_SWCtrlInfo.Trans.TxStatus.work_state = 0x01;
    } else if(s_SWCtrlInfo.runState == E_SW_RUN_STOP) {
        s_SWCtrlInfo.Trans.TxStatus.work_state = 0x00;
    }
    s_SWCtrlInfo.Trans.TxStatus.frequency = s_SWCtrlInfo.FreqLevel;
    s_SWCtrlInfo.Trans.TxStatus.remain_time = s_SWCtrlInfo.RemainPoints;
    s_SWCtrlInfo.Trans.TxStatus.work_level = s_SWCtrlInfo.WorkLevel;
    s_SWCtrlInfo.Trans.TxStatus.head_temp = s_SWCtrlInfo.HeadTemp;

    // Get connection state from treat mgr (probe + foot switch)
    bool headConnected = (App_TreatMgr_GetProbeStatus() == E_IODEVICE_MODE_SHOCKWAVE);
    bool footClosed = App_TreatMgr_GetFootSwitchClosed();
    if (headConnected && footClosed) {
        s_SWCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_CONNECTED_FOOT_CLOSED;
    } else if (!headConnected && footClosed) {
        s_SWCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_DISCONNECTED_FOOT_CLOSED;
    } else if (headConnected && !footClosed) {
        s_SWCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_CONNECTED_FOOT_OPEN;
    } else {
        s_SWCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_DISCONNECTED_FOOT_OPEN;
    }
    s_SWCtrlInfo.Trans.TxStatus.error_code = s_SWCtrlInfo.ErrorCode;
}

void App_Shockwave_RxDataHandle(void)
{
    SW_TransData_t *pTransData = App_Comm_GetSWTransData();
    s_SWCtrlInfo.Trans.RxWorkState = pTransData->RxWorkState;
}

void App_Shockwave_WorkTimeHandle(void)
{
    static uint8_t s_lastRxWorkState = 0x00;

    if(s_SWCtrlInfo.Trans.RxWorkState.work_state != s_lastRxWorkState) {
        if (s_SWCtrlInfo.Trans.RxWorkState.work_state == WORK_STATE_RESET && s_lastRxWorkState != WORK_STATE_RESET) {
            s_SWCtrlInfo.TreatCountsState = E_TREAT_TIMES_RESET;
        }
        s_lastRxWorkState = s_SWCtrlInfo.Trans.RxWorkState.work_state;
    }

    switch(s_SWCtrlInfo.TreatCountsState)
    {
        case E_TREAT_TIMES_POWER_ON:
            if(s_SWCtrlInfo.Trans.RxWorkState.work_time > 0 && s_SWCtrlInfo.TreatCounts > 0)
            {
                s_SWCtrlInfo.RemainPoints = s_SWCtrlInfo.Trans.RxWorkState.work_time;
                s_SWCtrlInfo.WorkLevel = s_SWCtrlInfo.Trans.RxWorkState.work_level;
                s_SWCtrlInfo.FreqLevel = s_SWCtrlInfo.Trans.RxWorkState.frequency;
                s_SWCtrlInfo.TreatCountsState = E_TREAT_TIMES_WORKING;
                s_SWCtrlInfo.TreatCounts--;
                s_SWCtrlInfo.TreatParams.TreatRemainTimes = s_SWCtrlInfo.TreatCounts;
                App_Memory_SaveSWParams(&s_SWCtrlInfo.TreatParams);
                LOG_I("SW: Remaining treat times decreased to: %d", s_SWCtrlInfo.TreatCounts);
            }
            break;
        case E_TREAT_TIMES_RESET:
            if(s_SWCtrlInfo.Trans.RxWorkState.work_time > 0 && s_SWCtrlInfo.TreatCounts > 0)
            {
                s_SWCtrlInfo.RemainPoints = s_SWCtrlInfo.Trans.RxWorkState.work_time;
                s_SWCtrlInfo.WorkLevel = s_SWCtrlInfo.Trans.RxWorkState.work_level;
                s_SWCtrlInfo.FreqLevel = s_SWCtrlInfo.Trans.RxWorkState.frequency;
                s_SWCtrlInfo.TreatCountsState = E_TREAT_TIMES_WORKING;
                s_SWCtrlInfo.TreatCounts--;
                s_SWCtrlInfo.TreatParams.TreatRemainTimes = s_SWCtrlInfo.TreatCounts;
                App_Memory_SaveSWParams(&s_SWCtrlInfo.TreatParams);
                LOG_I("SW: Remaining treat times decreased to: %d", s_SWCtrlInfo.TreatCounts);
            }
            break;
        case E_TREAT_TIMES_WORKING:
            if(s_SWCtrlInfo.RemainPoints == 0)
            {
                s_SWCtrlInfo.TreatCountsState = E_TREAT_TIMES_WAIT;
            }
            break;
        case E_TREAT_TIMES_WAIT:
            break;
        default:
            break;
    }
}

void App_Shockwave_ChangeState(SW_RunState_EnumDef newState)
{
    if(newState != s_SWCtrlInfo.runState && newState < E_SW_RUN_MAX)
    {
        s_SWCtrlInfo.runState = newState;
        switch(newState)
        {
            case E_SW_RUN_INIT:
                LOG_I("SW state changed to INIT");
                break;
            case E_SW_RUN_IDLE:
                LOG_I("SW state changed to IDLE");
                break;
            case E_SW_RUN_WORKING:
                LOG_I("SW state changed to WORKING");
                // Start buzzer for 2s when entering working
                Drv_IODevice_StartBuzzer(2000);
                break;
            case E_SW_RUN_STOP:
                LOG_I("SW state changed to STOP");
                // Start buzzer for 2s when stopping
                Drv_IODevice_StartBuzzer(2000);
                break;
            default:
                break;
        }
    }
}

void App_Shockwave_Monitor(void)
{
    // Reserved for treat mgr monitoring / extended checks
}

bool App_Shockwave_StartCheck()
{

    if(s_SWCtrlInfo.Trans.RxWorkState.work_state != WORK_STATE_START) {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        s_SWCtrlInfo.LastStartState = 0x00;
        return false;
    }

    if(s_SWCtrlInfo.Trans.RxWorkState.work_time == 0 || s_SWCtrlInfo.Trans.RxWorkState.work_time > SW_WORK_POINT_MAX) {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        s_SWCtrlInfo.LastStartState = 0x01;
        return false;
    }

    if(s_SWCtrlInfo.Trans.RxWorkState.work_level == 0 || s_SWCtrlInfo.Trans.RxWorkState.work_level > SW_WORK_LEVEL_MAX) {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        s_SWCtrlInfo.LastStartState = 0x02;
        return false;
    }

    if(s_SWCtrlInfo.Trans.RxWorkState.frequency == 0 || s_SWCtrlInfo.Trans.RxWorkState.frequency > SW_FREQ_LEVEL_MAX) {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        s_SWCtrlInfo.LastStartState = 0x03;
        return false;
    }

    if(App_TreatMgr_GetFootSwitchClosed()) {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        s_SWCtrlInfo.LastStartState = 0x04;
        return false;
    }

    if(App_TreatMgr_GetProbeStatus() != E_IODEVICE_MODE_SHOCKWAVE) {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_PROBE_NOT_CONNECTED;
        s_SWCtrlInfo.LastStartState = 0x05;
        return false;
    }

    if(s_SWCtrlInfo.TreatCounts == 0) {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        s_SWCtrlInfo.LastStartState = 0x06;
        return false;
    }

    if(s_SWCtrlInfo.RemainPoints == 0) {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        s_SWCtrlInfo.LastStartState = 0x07;
        return false;
    }

    LOG_I("SW: Start check passed");
    return true;
}

void App_Shockwave_SetWorkParams(void)
{
    // Set work/freq/points from Rx
    s_SWCtrlInfo.WorkLevel = s_SWCtrlInfo.Trans.RxWorkState.work_level;
    s_SWCtrlInfo.FreqLevel = s_SWCtrlInfo.Trans.RxWorkState.frequency;
    s_SWCtrlInfo.RemainPoints = s_SWCtrlInfo.Trans.RxWorkState.work_time;

    // Cycle period from frequency level
    s_SWCtrlInfo.cyclePeriodMs = App_Shockwave_CalculateCyclePeriod(s_SWCtrlInfo.FreqLevel);

    // PWM_ESW-N high time from work level
    s_SWCtrlInfo.pwmESW_NHighTimeMs = App_Shockwave_CalculateESW_NHighTime(s_SWCtrlInfo.WorkLevel);

    // Switch to READY channel (pwr_control1 etc.)
    Drv_IODevice_ChangeChannel(CHANNEL_READY);

	// Init PWM state
	s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_IDLE;
	s_SWCtrlInfo.cycleStartTime = 0;  // Will be set on first cycle
	Drv_GPIO_SetESW_P(false);
	Drv_GPIO_SetESW_N(false);

    LOG_I("SW: Work params set - level=%d, freq=%d, points=%d, period=%d ms, ESW_N_high=%d ms",
          s_SWCtrlInfo.WorkLevel, s_SWCtrlInfo.FreqLevel, s_SWCtrlInfo.RemainPoints,
          s_SWCtrlInfo.cyclePeriodMs, s_SWCtrlInfo.pwmESW_NHighTimeMs);
}

bool App_Shockwave_IsCurrentNormal(void)
{
    uint16_t current = Drv_ADC_GetRealValue(E_ADC_CHANNEL_ESW_I);
    bool isNormal = true;

    // Check current only when corresponding PWM is high
    if(s_SWCtrlInfo.pwmState == E_SW_PWM_STATE_ESW_P_HIGH)
    {
        // PWM_ESW+ high: check within ESW_P current range
        if(current < s_SWCtrlInfo.CurrentLow_ESW_P)
        {
            s_SWCtrlInfo.ErrorCode = E_SW_ERROR_CURRENT_ESW_P_LOW;
            LOG_W("SW: PWM_ESW+ current too low: %d (range: %d-%d)",
                  current, s_SWCtrlInfo.CurrentLow_ESW_P, s_SWCtrlInfo.CurrentHigh_ESW_P);
            isNormal = false;
        }
        else
        {
            s_SWCtrlInfo.ErrorCode = E_SW_ERROR_NONE;
        }
    }
    else if(s_SWCtrlInfo.pwmState == E_SW_PWM_STATE_ESW_N_HIGH)
    {
        // PWM_ESW-N high: check within ESW_N current range
        if(current < s_SWCtrlInfo.CurrentLow_ESW_N)
        {
            s_SWCtrlInfo.ErrorCode = E_SW_ERROR_CURRENT_ESW_N_LOW;
            LOG_W("SW: PWM_ESW- current too low: %d (range: %d-%d)",
                  current, s_SWCtrlInfo.CurrentLow_ESW_N, s_SWCtrlInfo.CurrentHigh_ESW_N);
            isNormal = false;
        }
        else
        {
            s_SWCtrlInfo.ErrorCode = E_SW_ERROR_NONE;
        }
    }

    return isNormal;
}

bool App_Shockwave_IsVoltageNormal(void)
{
    uint16_t voltage = Drv_ADC_GetRealValue(E_ADC_CHANNEL_ESW_U);
    bool isNormal = true;

    // Voltage threshold 3V (3000mV)
    if(voltage < SW_VOLTAGE_THRESHOLD_MV)
    {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_VOLTAGE_LOW;
        LOG_W("SW: Voltage too low: %d mV (threshold: %d mV)", voltage, SW_VOLTAGE_THRESHOLD_MV);
        isNormal = false;
    }
    else
    {
        if(s_SWCtrlInfo.ErrorCode == E_SW_ERROR_VOLTAGE_LOW)
        {
            s_SWCtrlInfo.ErrorCode = E_SW_ERROR_NONE;
        }
    }

    return isNormal;
}

bool App_Shockwave_IsHeadTempNormal(void)
{
    uint16_t temp = Drv_ADC_GetRealValue(E_ADC_CHANNEL_HAND_NTC);
    s_SWCtrlInfo.HeadTemp = temp;
    bool isNormal = true;

    if(temp > s_SWCtrlInfo.TempLimit)
    {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_TEMP_TOO_HIGH;
        LOG_W("SW: Head temperature too high: %d (limit: %d)", temp, s_SWCtrlInfo.TempLimit);
        // Over temp, stop treatment
        isNormal = false;
    }
    else
    {
        if(s_SWCtrlInfo.ErrorCode == E_SW_ERROR_TEMP_TOO_HIGH)
        {
            s_SWCtrlInfo.ErrorCode = E_SW_ERROR_NONE;
        }
    }

    return isNormal;
}

void App_Shockwave_ProcessPWM(void)
{
    uint32_t currentTime = Drv_Delay_GetTickMs();
    uint32_t elapsedTime;

    switch(s_SWCtrlInfo.pwmState)
    {
        case E_SW_PWM_STATE_IDLE:
            // Start new cycle when no cycle or cycle elapsed
            if(s_SWCtrlInfo.cycleStartTime == 0)
            {
                // First cycle or start of next cycle
                s_SWCtrlInfo.cycleStartTime = currentTime;
                s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_ESW_P_HIGH;
                s_SWCtrlInfo.pwmStateStartTime = currentTime;
                Drv_GPIO_SetESW_P(true);
                Drv_GPIO_SetESW_N(false);
            }
            else
            {
                // Check if current cycle period elapsed
                uint32_t cycleElapsed = currentTime - s_SWCtrlInfo.cycleStartTime;
                if(cycleElapsed >= s_SWCtrlInfo.cyclePeriodMs)
                {
                    // New cycle: start ESW_P high
                    s_SWCtrlInfo.cycleStartTime = currentTime;
                    s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_ESW_P_HIGH;
                    s_SWCtrlInfo.pwmStateStartTime = currentTime;
                    Drv_GPIO_SetESW_P(true);
                    Drv_GPIO_SetESW_N(false);
                    s_SWCtrlInfo.RemainPoints--;  // One point per cycle
                }
                // Else remain idle until next cycle
            }
            break;

        case E_SW_PWM_STATE_ESW_P_HIGH:
            elapsedTime = currentTime - s_SWCtrlInfo.pwmStateStartTime;
            if(elapsedTime >= SW_PWM_ESW_P_HIGH_TIME_MS)
            {
                // PWM_ESW+ high 5ms done, turn off and enter wait
                Drv_GPIO_SetESW_P(false);
                s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_WAIT;
                s_SWCtrlInfo.pwmStateStartTime = currentTime;
            }
            break;

        case E_SW_PWM_STATE_WAIT:
            elapsedTime = currentTime - s_SWCtrlInfo.pwmStateStartTime;
            if(elapsedTime >= SW_PWM_ESW_P_WAIT_TIME_MS)
            {
                // After 17ms wait, turn on PWM_ESW-N
                Drv_GPIO_SetESW_N(true);
                s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_ESW_N_HIGH;
                s_SWCtrlInfo.pwmStateStartTime = currentTime;
            }
            break;

        case E_SW_PWM_STATE_ESW_N_HIGH:
            elapsedTime = currentTime - s_SWCtrlInfo.pwmStateStartTime;
            if(elapsedTime >= s_SWCtrlInfo.pwmESW_NHighTimeMs)
            {
                // PWM_ESW-N high time elapsed, turn off
                Drv_GPIO_SetESW_N(false);
                // Back to IDLE for next cycle
                s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_IDLE;
            }
            break;

        default:
            break;
    }
}

void App_ShockWave_CheckProbe()
{
    static uint16_t debounceCount = 0;
    if(App_TreatMgr_GetProbeStatus() != E_IODEVICE_MODE_SHOCKWAVE) {
        debounceCount++;
        if(debounceCount >= PROBE_STATUS_DEBOUNCE_CNT) {
            debounceCount = 0;
            s_SWCtrlInfo.isWaitReturn = true;
        }
    } else {
        debounceCount = 0;
    }

    if(s_SWCtrlInfo.isWaitReturn) {
        App_Shockwave_ChangeState(E_SW_RUN_STOP);
    }
}
void App_Shockwave_Process(void)
{
    // Process the shockwave module
    App_Shockwave_UpdateStatus();
    App_Shockwave_RxDataHandle();
    App_Shockwave_WorkTimeHandle();
    App_Shockwave_Monitor();
    App_ShockWave_CheckProbe();
    // Handle the shockwave state
    switch(s_SWCtrlInfo.runState)
    {
        case E_SW_RUN_INIT:
            s_SWCtrlInfo.TreatCountsState = E_TREAT_TIMES_POWER_ON;
            {
                const SW_TreatParams_t *pParams = App_Memory_GetSWParams();
                s_SWCtrlInfo.TreatParams = *pParams;
                s_SWCtrlInfo.TempLimit = pParams->TempLimit;
                s_SWCtrlInfo.TreatCounts = pParams->TreatRemainTimes;
                s_SWCtrlInfo.CurrentHigh_ESW_P = pParams->CurrentHigh_ESW_P;
                s_SWCtrlInfo.CurrentLow_ESW_P = pParams->CurrentLow_ESW_P;
                s_SWCtrlInfo.CurrentHigh_ESW_N = pParams->CurrentHigh_ESW_N;
                s_SWCtrlInfo.CurrentLow_ESW_N = pParams->CurrentLow_ESW_N;

                LOG_I("SW: Parameters loaded - temp_limit=%d, remain_times=%d, ESW_P=[%d, %d], ESW_N=[%d, %d]",
                      s_SWCtrlInfo.TempLimit, s_SWCtrlInfo.TreatCounts,
                      s_SWCtrlInfo.CurrentLow_ESW_P, s_SWCtrlInfo.CurrentHigh_ESW_P,
                      s_SWCtrlInfo.CurrentLow_ESW_N, s_SWCtrlInfo.CurrentHigh_ESW_N);
            }
            // Switch to SW channel (pwr_control3/4 etc.)
            Drv_IODevice_ChangeChannel(CHANNEL_SW);
            App_Shockwave_ChangeState(E_SW_RUN_IDLE);
            break;

        case E_SW_RUN_IDLE:
            // If StartCheck passes, set params and start working
            if(App_Shockwave_StartCheck()) {
                App_Shockwave_SetWorkParams();
                Drv_IODevice_ChangeChannel(CHANNEL_READY);
                App_Shockwave_ChangeState(E_SW_RUN_WORKING);
            }
            break;

        case E_SW_RUN_WORKING:
            // Check all conditions (align with US)
            if(App_Shockwave_StartCheck() == false ||
               s_SWCtrlInfo.RemainPoints == 0 ||
               App_Shockwave_IsCurrentNormal() == false ||
               App_Shockwave_IsVoltageNormal() == false ||
               App_Shockwave_IsHeadTempNormal() == false) {
                App_Shockwave_ChangeState(E_SW_RUN_STOP);
            } else {
                App_Shockwave_ProcessPWM();
            }
            break;

        case E_SW_RUN_STOP:
            // Stop PWM output
            Drv_GPIO_SetESW_P(false);
            Drv_GPIO_SetESW_N(false);
            s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_IDLE;
            s_SWCtrlInfo.cycleStartTime = 0;
            Drv_IODevice_ChangeChannel(CHANNEL_CLOSE);
            if(s_SWCtrlInfo.isWaitReturn)
            {
                App_Shockwave_ChangeState(E_SW_RUN_WAIT_RETURN);
                LOG_I("SW: Wait return");
                s_SWCtrlInfo.isWaitReturn = false;
            }
            break;
        case E_SW_RUN_WAIT_RETURN:
            break;
        default:
            break;
    }
}

/**
 * @brief Initialize shockwave module
 */
void App_Shockwave_Init(void)
{
    memset(&s_SWCtrlInfo, 0, sizeof(SW_CtrlInfo_t));
    s_SWCtrlInfo.runState = E_SW_RUN_INIT;
    s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_IDLE;
    s_SWCtrlInfo.ErrorCode = E_SW_ERROR_NONE;
    s_SWCtrlInfo.WorkLevel = 0;
    s_SWCtrlInfo.FreqLevel = 0;
    s_SWCtrlInfo.RemainPoints = 0;
    s_SWCtrlInfo.TreatCounts = 0;
    /* ESW_P/ESW_N (PB8/PB9) init in BSP_Init -> BSP_GPIO_Init */
    Drv_GPIO_SetESW_P(false);
    Drv_GPIO_SetESW_N(false);

    LOG_I("Shockwave module initialized");
}

SW_GetStatus_Reply_t *App_Shockwave_GetStatus(void)
{
    return &s_SWCtrlInfo.Trans.TxStatus;
}


SW_RunState_EnumDef App_Shockwave_GetRunState(void)
{
    return s_SWCtrlInfo.runState;
}

/**************************End of file********************************/
