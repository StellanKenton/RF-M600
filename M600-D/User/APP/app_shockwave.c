/***********************************************************************************
* @file     : app_shockwave.c
* @brief    : Shock Wave treatment module implementation
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
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
 * @brief Calculate cycle period from frequency level (1-16????1S??62.5ms)
 * @param freqLevel Frequency level (1-16)
 * @retval Cycle period in milliseconds
 */
static uint32_t App_Shockwave_CalculateCyclePeriod(uint8_t freqLevel)
{
    if(freqLevel == 0 || freqLevel > SW_FREQ_LEVEL_MAX) {
        freqLevel = 1;
    }
    // ?? = 1000ms / ??
    // ??1: 1000ms, ??16: 62.5ms
    return 1000 / freqLevel;
}

/**
 * @brief Calculate PWM_ESW- high time from work level
 * @param level Work level (1-26)
 * @retval High time in milliseconds (??????????0.28ms??)
 */
static uint32_t App_Shockwave_CalculateESW_NHighTime(uint8_t level)
{
    if(level == 0) {
        return 0;
    }
    if(level > SW_WORK_LEVEL_MAX) {
        level = SW_WORK_LEVEL_MAX;
    }
    // ?????? = 3 + (level - 1) * 0.28 ms
    // ????????3000 + (level - 1) * 280 (????0.001ms??????)
    // ????1000????????????????
    // 3ms = 3000????0.28ms = 280????
    uint32_t time_us = 3000 + (level - 1) * 280;  // ????
    return (time_us + 500) / 1000;  // ??????????
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
    
    // ???????? mgr ??
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
    
    if(pTransData->RxWorkState.work_state == WORK_STATE_RESET)
    {
        // ??????
        // ????????????????????
        s_SWCtrlInfo.RemainPoints = pTransData->RxWorkState.work_time;  // work_time??????
        s_SWCtrlInfo.WorkLevel = pTransData->RxWorkState.work_level;
        s_SWCtrlInfo.FreqLevel = pTransData->RxWorkState.frequency;
        
        // ??????????
        if(s_SWCtrlInfo.TreatCounts > 0)
        {
            s_SWCtrlInfo.TreatCounts--;
            // ??????
            s_SWCtrlInfo.TreatParams.TreatRemainTimes = s_SWCtrlInfo.TreatCounts;
            App_Memory_SaveSWParams(&s_SWCtrlInfo.TreatParams);
            LOG_I("SW Reset: Remaining treat times decreased to: %d", s_SWCtrlInfo.TreatCounts);
        }
        
        LOG_I("SW Reset: Work points=%d, Work level=%d, Freq level=%d", 
              s_SWCtrlInfo.RemainPoints, s_SWCtrlInfo.WorkLevel, s_SWCtrlInfo.FreqLevel);
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
                // ????????????2s??
                Drv_IODevice_StartBuzzer(2000);
                break;
            case E_SW_RUN_STOP:
                LOG_I("SW state changed to STOP");
                // ????????????2s??
                Drv_IODevice_StartBuzzer(2000);
                break;
            default:
                break;
        }
    }
}

void App_Shockwave_Monitor(void)
{
    // ???????? mgr ?????
}

bool App_Shockwave_StartCheck()
{
    SW_TransData_t *pTransData = App_Comm_GetSWTransData();
    
    // 1. ???????????????????
    if(pTransData->RxWorkState.work_state != WORK_STATE_START) {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 2. ?????????????0??0-10000??
    if(pTransData->RxWorkState.work_time == 0 || pTransData->RxWorkState.work_time > SW_WORK_POINT_MAX) {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 3. ???????????0??0-26??
    if(pTransData->RxWorkState.work_level == 0 || pTransData->RxWorkState.work_level > SW_WORK_LEVEL_MAX) {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 4. ????????????????1-16??
    if(pTransData->RxWorkState.frequency == 0 || pTransData->RxWorkState.frequency > SW_FREQ_LEVEL_MAX) {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 5. ???????????
    if(!App_TreatMgr_GetFootSwitchClosed()) {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 6. ??????????????????
    if(App_TreatMgr_GetProbeStatus() != E_IODEVICE_MODE_SHOCKWAVE) {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_PROBE_NOT_CONNECTED;
        return false;
    }
    
    // 7. ?????????????
    if(s_SWCtrlInfo.TreatCounts == 0) {
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // ????????
    LOG_I("SW: Start check passed");
    return true;
}

void App_Shockwave_SetWorkParams(void)
{
    SW_TransData_t *pTransData = App_Comm_GetSWTransData();
    
    // ??????
    s_SWCtrlInfo.WorkLevel = pTransData->RxWorkState.work_level;
    s_SWCtrlInfo.FreqLevel = pTransData->RxWorkState.frequency;
    s_SWCtrlInfo.RemainPoints = pTransData->RxWorkState.work_time;
    
    // ??????
    s_SWCtrlInfo.cyclePeriodMs = App_Shockwave_CalculateCyclePeriod(s_SWCtrlInfo.FreqLevel);
    
    // ??PWM_ESW-??????
    s_SWCtrlInfo.pwmESW_NHighTimeMs = App_Shockwave_CalculateESW_NHighTime(s_SWCtrlInfo.WorkLevel);
    
    // ?????pwr_control1??????????????????????????????????????
    Drv_IODevice_ChangeChannel(CHANNEL_READY);
    
	// ?????PWM???
	s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_IDLE;
	s_SWCtrlInfo.cycleStartTime = 0;  // ?????????
	Drv_TIM4_SetESW_P(false);
	Drv_TIM4_SetESW_N(false);
    
    LOG_I("SW: Work params set - level=%d, freq=%d, points=%d, period=%d ms, ESW_N_high=%d ms", 
          s_SWCtrlInfo.WorkLevel, s_SWCtrlInfo.FreqLevel, s_SWCtrlInfo.RemainPoints,
          s_SWCtrlInfo.cyclePeriodMs, s_SWCtrlInfo.pwmESW_NHighTimeMs);
}

bool App_Shockwave_IsCurrentNormal(void)
{
    uint16_t current = Drv_ADC_GetRealValue(E_ADC_CHANNEL_ESW_I);
    bool isNormal = true;
    
    // ????PWM?????????
    if(s_SWCtrlInfo.pwmState == E_SW_PWM_STATE_ESW_P_HIGH)
    {
        // PWM_ESW+?????????????PWM_ESW+??????
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
        // PWM_ESW-?????????????PWM_ESW-??????
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
    
    // ??????3V????
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
        // ???????????????
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
            // ???????????
            if(s_SWCtrlInfo.cycleStartTime == 0)
            {
                // ?????????????
                s_SWCtrlInfo.cycleStartTime = currentTime;
                s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_ESW_P_HIGH;
                s_SWCtrlInfo.pwmStateStartTime = currentTime;
                Drv_TIM4_SetESW_P(true);
                Drv_TIM4_SetESW_N(false);
            }
            else
            {
                // ?????????
                uint32_t cycleElapsed = currentTime - s_SWCtrlInfo.cycleStartTime;
                if(cycleElapsed >= s_SWCtrlInfo.cyclePeriodMs)
                {
                    // ??????????
                    s_SWCtrlInfo.cycleStartTime = currentTime;
                    s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_ESW_P_HIGH;
                    s_SWCtrlInfo.pwmStateStartTime = currentTime;
                    Drv_TIM4_SetESW_P(true);
                    Drv_TIM4_SetESW_N(false);
                    s_SWCtrlInfo.RemainPoints--;  // ??????
                }
                // ??????
            }
            break;
            
        case E_SW_PWM_STATE_ESW_P_HIGH:
            elapsedTime = currentTime - s_SWCtrlInfo.pwmStateStartTime;
            if(elapsedTime >= SW_PWM_ESW_P_HIGH_TIME_MS)
            {
                // PWM_ESW+????5ms????????????????
                Drv_TIM4_SetESW_P(false);
                s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_WAIT;
                s_SWCtrlInfo.pwmStateStartTime = currentTime;
            }
            break;
            
        case E_SW_PWM_STATE_WAIT:
            elapsedTime = currentTime - s_SWCtrlInfo.pwmStateStartTime;
            if(elapsedTime >= SW_PWM_ESW_P_WAIT_TIME_MS)
            {
                // ??17ms??PWM_ESW-????
                Drv_TIM4_SetESW_N(true);
                s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_ESW_N_HIGH;
                s_SWCtrlInfo.pwmStateStartTime = currentTime;
            }
            break;
            
        case E_SW_PWM_STATE_ESW_N_HIGH:
            elapsedTime = currentTime - s_SWCtrlInfo.pwmStateStartTime;
            if(elapsedTime >= s_SWCtrlInfo.pwmESW_NHighTimeMs)
            {
                // PWM_ESW-??????????????
                Drv_TIM4_SetESW_N(false);
                
                // PWM_ESW-??????????????
                // ??????????????
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
    static Drv_Timer_t TempMonitorTimer;
    
    // Process the shockwave module
    App_Shockwave_UpdateStatus();
    App_Shockwave_RxDataHandle();
    App_Shockwave_Monitor();
    App_ShockWave_CheckProbe();
    // Handle the shockwave state
    switch(s_SWCtrlInfo.runState)
    {
        case E_SW_RUN_INIT:
            // ????????
            if(App_Memory_LoadSWParams(&s_SWCtrlInfo.TreatParams)) {
                s_SWCtrlInfo.TempLimit = s_SWCtrlInfo.TreatParams.TempLimit;
                s_SWCtrlInfo.TreatCounts = s_SWCtrlInfo.TreatParams.TreatRemainTimes;
                s_SWCtrlInfo.CurrentHigh_ESW_P = s_SWCtrlInfo.TreatParams.CurrentHigh_ESW_P;
                s_SWCtrlInfo.CurrentLow_ESW_P = s_SWCtrlInfo.TreatParams.CurrentLow_ESW_P;
                s_SWCtrlInfo.CurrentHigh_ESW_N = s_SWCtrlInfo.TreatParams.CurrentHigh_ESW_N;
                s_SWCtrlInfo.CurrentLow_ESW_N = s_SWCtrlInfo.TreatParams.CurrentLow_ESW_N;
                
                LOG_I("SW: Parameters loaded - temp_limit=%d, remain_times=%d, ESW_P=[%d, %d], ESW_N=[%d, %d]",
                      s_SWCtrlInfo.TempLimit, s_SWCtrlInfo.TreatCounts,
                      s_SWCtrlInfo.CurrentLow_ESW_P, s_SWCtrlInfo.CurrentHigh_ESW_P,
                      s_SWCtrlInfo.CurrentLow_ESW_N, s_SWCtrlInfo.CurrentHigh_ESW_N);
            } else {
                LOG_E("SW: Failed to load parameters");
                s_SWCtrlInfo.ErrorCode = E_SW_ERROR_READ_PARAMS_FAILED;
            }
            App_Shockwave_ChangeState(E_SW_RUN_IDLE);
            break;
            
        case E_SW_RUN_IDLE:
            // ??App_Shockwave_StartCheck??????????
            if(App_Shockwave_StartCheck()) {
                // ??????????????
                App_Shockwave_SetWorkParams();

                // pwr_control3?pwr_control4????????????????
                Drv_IODevice_ChangeChannel(CHANNEL_SW);
                App_Shockwave_ChangeState(E_SW_RUN_WORKING);
            }
            break;
            
        case E_SW_RUN_WORKING:
            
            // ???????
            if(App_Shockwave_StartCheck() == false || 
               s_SWCtrlInfo.RemainPoints == 0){
                App_Shockwave_ChangeState(E_SW_RUN_STOP);
            } else {
                // ??PWM??
                App_Shockwave_ProcessPWM();
                
                // ??????PWM??????
                if(s_SWCtrlInfo.pwmState == E_SW_PWM_STATE_ESW_P_HIGH || 
                   s_SWCtrlInfo.pwmState == E_SW_PWM_STATE_ESW_N_HIGH) {
                    if(App_Shockwave_IsCurrentNormal() == false) {
                        // ??????????????
                    }
                }
                
                // ????
                if(App_Shockwave_IsVoltageNormal() == false) {
                    // ??????????????
                }
                
                // ??????10ms????
                if(Drv_Timer_Tick(&TempMonitorTimer, SW_TEMP_MONITOR_PERIOD_MS)) {
                    if(App_Shockwave_IsHeadTempNormal() == false) {
                        // ???????????????
                        App_Shockwave_ChangeState(E_SW_RUN_STOP);
                    }
                }
            }
            break;
            
        case E_SW_RUN_STOP:
            // ??PWM??
            Drv_TIM4_SetESW_P(false);
            Drv_TIM4_SetESW_N(false);
            // ??PWM???
            s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_IDLE;
            s_SWCtrlInfo.cycleStartTime = 0;
            // ??????
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
    // ???????????
    memset(&s_SWCtrlInfo, 0, sizeof(SW_CtrlInfo_t));
    
    // ?????????
    s_SWCtrlInfo.runState = E_SW_RUN_INIT;
    s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_IDLE;
    s_SWCtrlInfo.ErrorCode = E_SW_ERROR_NONE;
    s_SWCtrlInfo.WorkLevel = 0;
    s_SWCtrlInfo.FreqLevel = 0;
    s_SWCtrlInfo.RemainPoints = 0;
    s_SWCtrlInfo.TreatCounts = 0;
    
    /* TIM4 ?? BSP_Init -> BSP_TIM4_Init ??????? */
    /* ???PWM?????? */
    Drv_TIM4_SetESW_P(false);
    Drv_TIM4_SetESW_N(false);
    
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
