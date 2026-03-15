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

static void App_Shockwave_ResetPulseEngine(void)
{
    Drv_GPIO_SetESW_P(false);
    Drv_GPIO_SetESW_N(false);
    s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_IDLE;
    s_SWCtrlInfo.pwmStateStartTimeUs = 0U;
    s_SWCtrlInfo.cycleStartTimeUs = 0U;
    s_SWCtrlInfo.nextCycleStartTimeUs = 0U;
}

/**
 * @brief Calculate cycle period from frequency level (1-16; level1=1000000us, level16=62500us)
 * @param freqLevel Frequency level (1-16)
 * @retval Cycle period in microseconds
 */
static uint32_t App_Shockwave_CalculateCyclePeriodUs(uint8_t freqLevel)
{
    if(freqLevel == 0 || freqLevel > SW_FREQ_LEVEL_MAX) {
        freqLevel = 1;
    }
    return 1000000U / freqLevel;
}

/**
 * @brief Calculate PWM_ESW-N high time from work level
 * @param level Work level (1-26)
 * @retval High time in microseconds
 */
static uint32_t App_Shockwave_CalculateESW_NHighTimeUs(uint8_t level)
{
    if(level == 0) {
        return 0;
    }
    if(level > SW_WORK_LEVEL_MAX) {
        level = SW_WORK_LEVEL_MAX;
    }
    return SW_PWM_ESW_N_BASE_TIME_US + (uint32_t)(level - 1U) * SW_PWM_ESW_N_STEP_TIME_US;
}

void App_Shockwave_UpdateStatus(void)
{
    // Update work state
    if(s_SWCtrlInfo.runState == E_SW_RUN_WORKING) {
        s_SWCtrlInfo.Trans.TxStatus.work_state = 0x01;
    } else {
        s_SWCtrlInfo.Trans.TxStatus.work_state = 0x00;
    }
    s_SWCtrlInfo.Trans.TxStatus.frequency = s_SWCtrlInfo.FreqLevel;
    s_SWCtrlInfo.Trans.TxStatus.remain_time = s_SWCtrlInfo.RemainPoints;
    s_SWCtrlInfo.Trans.TxStatus.work_level = s_SWCtrlInfo.WorkLevel;
    s_SWCtrlInfo.Trans.TxStatus.head_temp = s_SWCtrlInfo.HeadTemp;
    s_SWCtrlInfo.Trans.TxStatus.remain_treatment_count = (uint16_t)s_SWCtrlInfo.TreatCounts;

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
        SW_RunState_EnumDef oldState = s_SWCtrlInfo.runState;
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
                // Only beep when output really stops, not when a module switch forces STOP from IDLE.
                if(oldState == E_SW_RUN_WORKING) {
                    Drv_IODevice_StartBuzzer(2000);
                }
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

    return true;
}

void App_Shockwave_SetWorkParams(void)
{
    // Set work/freq/points from Rx
    s_SWCtrlInfo.WorkLevel = s_SWCtrlInfo.Trans.RxWorkState.work_level;
    s_SWCtrlInfo.FreqLevel = s_SWCtrlInfo.Trans.RxWorkState.frequency;

    // Cycle period from frequency level
    s_SWCtrlInfo.cyclePeriodUs = App_Shockwave_CalculateCyclePeriodUs(s_SWCtrlInfo.FreqLevel);

    // PWM_ESW-N high time from work level
    s_SWCtrlInfo.pwmESW_NHighTimeUs = App_Shockwave_CalculateESW_NHighTimeUs(s_SWCtrlInfo.WorkLevel);

    // Switch to READY channel (pwr_control1 etc.)
    Drv_IODevice_ChangeChannel(CHANNEL_READY);

    App_Shockwave_ResetPulseEngine();

    LOG_I("SW: Work params set - level=%d, freq=%d, points=%d, period=%lu us, ESW_N_high=%lu us",
          s_SWCtrlInfo.WorkLevel, s_SWCtrlInfo.FreqLevel, s_SWCtrlInfo.RemainPoints,
          (unsigned long)s_SWCtrlInfo.cyclePeriodUs, (unsigned long)s_SWCtrlInfo.pwmESW_NHighTimeUs);
}

bool App_Shockwave_IsCurrentNormal(void)
{
    uint16_t current = Drv_ADC_GetRealValue(BSP_ADC_CH_ESW_I);
    bool isNormal = true;

    if(TreatGetRunFlag()) {
        return true;
    }
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
    static uint32_t s_voltageLowStartTime = 0;
    static bool s_voltageLowPending = false;

    uint16_t voltage = Drv_ADC_GetRealValue(BSP_ADC_CH_ESW_U);
    uint32_t currentTime = Drv_Delay_GetTickMs();
    bool isNormal = true;
    
    if(TreatGetRunFlag()) {
        return true;
    }
    // Voltage must remain below threshold for 100 ms before it is treated as abnormal.
    if(voltage < SW_VOLTAGE_THRESHOLD_MV)
    {
        if(!s_voltageLowPending)
        {
            s_voltageLowPending = true;
            s_voltageLowStartTime = currentTime;
        }
        else if((currentTime - s_voltageLowStartTime) >= 100U)
        {
            s_SWCtrlInfo.ErrorCode = E_SW_ERROR_VOLTAGE_LOW;
            LOG_W("SW: Voltage too low: %d mV (threshold: %d mV)", voltage, SW_VOLTAGE_THRESHOLD_MV);
            isNormal = false;
        }
    }
    else
    {
        s_voltageLowPending = false;
        s_voltageLowStartTime = 0;

        if(s_SWCtrlInfo.ErrorCode == E_SW_ERROR_VOLTAGE_LOW)
        {
            s_SWCtrlInfo.ErrorCode = E_SW_ERROR_NONE;
        }
    }

    return isNormal;
}

bool App_Shockwave_IsHeadTempNormal(void)
{
    uint16_t temp = Drv_ADC_GetRealValue(BSP_ADC_CH_HAND_NTC);
    s_SWCtrlInfo.HeadTemp = temp;
    bool isNormal = true;
    
    if(TreatGetRunFlag()) {
        return true;
    }
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

void App_Shockwave_TimerTick100us(void)
{
    uint64_t nowUs;

    if(s_SWCtrlInfo.runState != E_SW_RUN_WORKING || s_SWCtrlInfo.RemainPoints == 0U) {
        App_Shockwave_ResetPulseEngine();
        return;
    }

    nowUs = Drv_GetSystemTickUs();

    switch(s_SWCtrlInfo.pwmState)
    {
        case E_SW_PWM_STATE_IDLE:
            if(s_SWCtrlInfo.nextCycleStartTimeUs == 0U) {
                s_SWCtrlInfo.nextCycleStartTimeUs = nowUs;
            }

            if(nowUs >= s_SWCtrlInfo.nextCycleStartTimeUs)
            {
                s_SWCtrlInfo.cycleStartTimeUs = nowUs;
                s_SWCtrlInfo.nextCycleStartTimeUs = nowUs + s_SWCtrlInfo.cyclePeriodUs;
                s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_ESW_P_HIGH;
                s_SWCtrlInfo.pwmStateStartTimeUs = nowUs;
                Drv_GPIO_SetESW_P(true);
                Drv_GPIO_SetESW_N(false);
                s_SWCtrlInfo.RemainPoints--;
            }
            break;

        case E_SW_PWM_STATE_ESW_P_HIGH:
            if((nowUs - s_SWCtrlInfo.pwmStateStartTimeUs) >= SW_PWM_ESW_P_HIGH_TIME_US)
            {
                Drv_GPIO_SetESW_P(false);
                s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_WAIT;
                s_SWCtrlInfo.pwmStateStartTimeUs = nowUs;
            }
            break;

        case E_SW_PWM_STATE_WAIT:
            if((nowUs - s_SWCtrlInfo.pwmStateStartTimeUs) >= SW_PWM_ESW_P_WAIT_TIME_US)
            {
                Drv_GPIO_SetESW_N(true);
                s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_ESW_N_HIGH;
                s_SWCtrlInfo.pwmStateStartTimeUs = nowUs;
            }
            break;

        case E_SW_PWM_STATE_ESW_N_HIGH:
            if((nowUs - s_SWCtrlInfo.pwmStateStartTimeUs) >= s_SWCtrlInfo.pwmESW_NHighTimeUs)
            {
                Drv_GPIO_SetESW_N(false);
                s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_IDLE;
                s_SWCtrlInfo.pwmStateStartTimeUs = 0U;
            }
            break;

        default:
            App_Shockwave_ResetPulseEngine();
            break;
    }
}

static void App_Shockwave_RunChangeLevel(void)
{
    bool workLevelChanged = false;
    bool freqLevelChanged = false;

    if((s_SWCtrlInfo.Trans.RxWorkState.work_level > 0) &&
       (s_SWCtrlInfo.Trans.RxWorkState.work_level <= SW_WORK_LEVEL_MAX) &&
       (s_SWCtrlInfo.WorkLevel != s_SWCtrlInfo.Trans.RxWorkState.work_level))
    {
        s_SWCtrlInfo.WorkLevel = s_SWCtrlInfo.Trans.RxWorkState.work_level;
        s_SWCtrlInfo.pwmESW_NHighTimeUs = App_Shockwave_CalculateESW_NHighTimeUs(s_SWCtrlInfo.WorkLevel);
        workLevelChanged = true;
    }

    if((s_SWCtrlInfo.Trans.RxWorkState.frequency > 0) &&
       (s_SWCtrlInfo.Trans.RxWorkState.frequency <= SW_FREQ_LEVEL_MAX) &&
       (s_SWCtrlInfo.FreqLevel != s_SWCtrlInfo.Trans.RxWorkState.frequency))
    {
        s_SWCtrlInfo.FreqLevel = s_SWCtrlInfo.Trans.RxWorkState.frequency;
        s_SWCtrlInfo.cyclePeriodUs = App_Shockwave_CalculateCyclePeriodUs(s_SWCtrlInfo.FreqLevel);
        freqLevelChanged = true;
    }

    if(workLevelChanged || freqLevelChanged)
    {
        LOG_I("SW: Run params updated - level=%d, freq=%d, period=%lu us, ESW_N_high=%lu us",
              s_SWCtrlInfo.WorkLevel, s_SWCtrlInfo.FreqLevel,
              (unsigned long)s_SWCtrlInfo.cyclePeriodUs, (unsigned long)s_SWCtrlInfo.pwmESW_NHighTimeUs);
    }
}

void App_ShockWave_CheckProbe()
{
    if(App_TreatMgr_GetProbeStatus() != E_IODEVICE_MODE_SHOCKWAVE) {
        s_SWCtrlInfo.isWaitReturn = true;
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
                App_Shockwave_RunChangeLevel();
            }
            break;

        case E_SW_RUN_STOP:
            // Stop PWM output
            App_Shockwave_ResetPulseEngine();
            Drv_IODevice_ChangeChannel(CHANNEL_CLOSE);
            App_Shockwave_ChangeState(E_SW_RUN_IDLE);
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
    App_Shockwave_ResetPulseEngine();

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
