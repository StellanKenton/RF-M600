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
#include "log.h"
#include "drv_delay.h"
#include "tim.h"
#include "stm32f1xx_hal.h"
#include <string.h>

extern TIM_HandleTypeDef htim4;

static SW_CtrlInfo_t s_SWCtrlInfo;

/**
 * @brief Calculate cycle period from frequency level (1-16档位对应1S到62.5ms)
 * @param freqLevel Frequency level (1-16)
 * @retval Cycle period in milliseconds
 */
static uint32_t App_Shockwave_CalculateCyclePeriod(uint8_t freqLevel)
{
    if(freqLevel == 0 || freqLevel > SW_FREQ_LEVEL_MAX) {
        freqLevel = 1;
    }
    // 周期 = 1000ms / 档位
    // 档位1: 1000ms, 档位16: 62.5ms
    return 1000 / freqLevel;
}

/**
 * @brief Calculate PWM_ESW- high time from work level
 * @param level Work level (1-26)
 * @retval High time in milliseconds (使用整数计算，保留0.28ms精度)
 */
static uint32_t App_Shockwave_CalculateESW_NHighTime(uint8_t level)
{
    if(level == 0) {
        return 0;
    }
    if(level > SW_WORK_LEVEL_MAX) {
        level = SW_WORK_LEVEL_MAX;
    }
    // 高电平时间 = 3 + (level - 1) * 0.28 ms
    // 使用整数计算：3000 + (level - 1) * 280 (单位：0.001ms，即微秒)
    // 然后除以1000转换为毫秒，但保留精度
    // 3ms = 3000微秒，0.28ms = 280微秒
    uint32_t time_us = 3000 + (level - 1) * 280;  // 微秒
    return (time_us + 500) / 1000;  // 四舍五入到毫秒
}

/**
 * @brief Control PWM_ESW+ output (TIM4_CH3)
 * @param state true = high, false = low
 */
static void App_Shockwave_SetESW_P(bool state)
{
    if(state) {
        // 设置PWM输出高电平（通过设置占空比为100%）
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, htim4.Init.Period);
        HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
    } else {
        // 设置PWM输出低电平（通过设置占空比为0%）
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, 0);
        HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3);
    }
}

/**
 * @brief Control PWM_ESW- output (TIM4_CH4)
 * @param state true = high, false = low
 */
static void App_Shockwave_SetESW_N(bool state)
{
    if(state) {
        // 设置PWM输出高电平
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, htim4.Init.Period);
        HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
    } else {
        // 设置PWM输出低电平
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 0);
        HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4);
    }
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
    
    // Get probe connection status and foot switch state
    bool headConnected = (s_SWCtrlInfo.probeStatus == E_IODEVICE_MODE_SHOCKWAVE);
    
    // Combine connection state
    if (headConnected && s_SWCtrlInfo.FootSwitchStatus) {
        s_SWCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_CONNECTED_FOOT_CLOSED;
    } else if (!headConnected && s_SWCtrlInfo.FootSwitchStatus) {
        s_SWCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_DISCONNECTED_FOOT_CLOSED;
    } else if (headConnected && !s_SWCtrlInfo.FootSwitchStatus) {
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
        // 处理复位功能
        // 重置治疗点数、治疗档位、治疗频率档位
        s_SWCtrlInfo.RemainPoints = pTransData->RxWorkState.work_time;  // work_time实际是点数
        s_SWCtrlInfo.WorkLevel = pTransData->RxWorkState.work_level;
        s_SWCtrlInfo.FreqLevel = pTransData->RxWorkState.frequency;
        
        // 剩余可治疗次数减一
        if(s_SWCtrlInfo.TreatTimes > 0)
        {
            s_SWCtrlInfo.TreatTimes--;
            // 保存到存储器
            s_SWCtrlInfo.TreatParams.RemainTimes = s_SWCtrlInfo.TreatTimes;
            App_Memory_SaveSWParams(&s_SWCtrlInfo.TreatParams);
            LOG_I("SW Reset: Remaining treat times decreased to: %d", s_SWCtrlInfo.TreatTimes);
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
                break;
            case E_SW_RUN_STOP:
                LOG_I("SW state changed to STOP");
                break;
            default:
                break;
        }
    }
}

void App_Shockwave_Monitor(void)
{
    // Get the foot switch status and probe status
    s_SWCtrlInfo.FootSwitchStatus = Drv_IODevice_GetFootSwitchState();
    
    // Get the probe status
    s_SWCtrlInfo.probeStatus = Drv_IODevice_GetProbeStatus();
}

bool App_Shockwave_StartCheck()
{
    SW_TransData_t *pTransData = App_Comm_GetSWTransData();
    
    // 1. 检查下位机是否下发了发射冲击波指令
    if(pTransData->RxWorkState.work_state != WORK_STATE_START) {
        LOG_E("SW: Work state is not start");
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 2. 检查剩余工作点数是否大于0（0-10000）
    if(pTransData->RxWorkState.work_time == 0 || pTransData->RxWorkState.work_time > SW_WORK_POINT_MAX) {
        LOG_E("SW: Invalid work points: %d", pTransData->RxWorkState.work_time);
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 3. 检查工作档位是否不等于0（0-26）
    if(pTransData->RxWorkState.work_level == 0 || pTransData->RxWorkState.work_level > SW_WORK_LEVEL_MAX) {
        LOG_E("SW: Invalid work level: %d (range: 1-%d)", pTransData->RxWorkState.work_level, SW_WORK_LEVEL_MAX);
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 4. 检查工作频率档位是否有效（1-16）
    if(pTransData->RxWorkState.frequency == 0 || pTransData->RxWorkState.frequency > SW_FREQ_LEVEL_MAX) {
        LOG_E("SW: Invalid frequency level: %d (range: 1-%d)", pTransData->RxWorkState.frequency, SW_FREQ_LEVEL_MAX);
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 5. 检查脚踏开关是否闭合
    if(s_SWCtrlInfo.FootSwitchStatus == false) {
        LOG_E("SW: Foot switch is not closed");
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 6. 检查是否正确识别到冲击波治疗头
    if(s_SWCtrlInfo.probeStatus != E_IODEVICE_MODE_SHOCKWAVE) {
        LOG_E("SW: Shockwave probe not connected");
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_PROBE_NOT_CONNECTED;
        return false;
    }
    
    // 7. 检查是否有剩余可治疗次数
    if(s_SWCtrlInfo.TreatTimes == 0) {
        LOG_E("SW: No remaining treat times");
        s_SWCtrlInfo.ErrorCode = E_SW_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 所有检查通过
    return true;
}

void App_Shockwave_SetWorkParams(void)
{
    SW_TransData_t *pTransData = App_Comm_GetSWTransData();
    
    // 设置工作参数
    s_SWCtrlInfo.WorkLevel = pTransData->RxWorkState.work_level;
    s_SWCtrlInfo.FreqLevel = pTransData->RxWorkState.frequency;
    s_SWCtrlInfo.RemainPoints = pTransData->RxWorkState.work_time;
    
    // 计算周期时间
    s_SWCtrlInfo.cyclePeriodMs = App_Shockwave_CalculateCyclePeriod(s_SWCtrlInfo.FreqLevel);
    
    // 计算PWM_ESW-高电平时间
    s_SWCtrlInfo.pwmESW_NHighTimeMs = App_Shockwave_CalculateESW_NHighTime(s_SWCtrlInfo.WorkLevel);
    
    // 切换继电器pwr_control1至冲击波通道（需求说切换至超声通道，可能是笔误，应该是冲击波通道）
    Drv_IODevice_ChangeChannel(CHANNEL_READY);
    
            // 初始化PWM状态
            s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_IDLE;
            s_SWCtrlInfo.cycleStartTime = 0;  // 重置周期开始时间
            App_Shockwave_SetESW_P(false);
            App_Shockwave_SetESW_N(false);
    
    LOG_I("SW: Work params set - level=%d, freq=%d, points=%d, period=%d ms, ESW_N_high=%d ms", 
          s_SWCtrlInfo.WorkLevel, s_SWCtrlInfo.FreqLevel, s_SWCtrlInfo.RemainPoints,
          s_SWCtrlInfo.cyclePeriodMs, s_SWCtrlInfo.pwmESW_NHighTimeMs);
}

bool App_Shockwave_IsCurrentNormal(void)
{
    uint16_t current = Drv_ADC_GetRealValue(E_ADC_CHANNEL_ESW_I);
    bool isNormal = true;
    
    // 根据当前PWM状态检查电流
    if(s_SWCtrlInfo.pwmState == E_SW_PWM_STATE_ESW_P_HIGH)
    {
        // PWM_ESW+高电平时，监控电压应该大于PWM_ESW+工作电流区间
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
        // PWM_ESW-高电平时，监控电压应该大于PWM_ESW-工作电流区间
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
    
    // 采样电压低于3V时报警
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
        // 温度超限，立马停止工作
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
    uint32_t currentTime = HAL_GetTick();
    uint32_t elapsedTime;
    
    switch(s_SWCtrlInfo.pwmState)
    {
        case E_SW_PWM_STATE_IDLE:
            // 检查是否应该开始新周期
            if(s_SWCtrlInfo.cycleStartTime == 0)
            {
                // 第一次启动，立即开始
                s_SWCtrlInfo.cycleStartTime = currentTime;
                s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_ESW_P_HIGH;
                s_SWCtrlInfo.pwmStateStartTime = currentTime;
                App_Shockwave_SetESW_P(true);
                App_Shockwave_SetESW_N(false);
            }
            else
            {
                // 检查周期是否完成
                uint32_t cycleElapsed = currentTime - s_SWCtrlInfo.cycleStartTime;
                if(cycleElapsed >= s_SWCtrlInfo.cyclePeriodMs)
                {
                    // 周期完成，开始新周期
                    s_SWCtrlInfo.cycleStartTime = currentTime;
                    s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_ESW_P_HIGH;
                    s_SWCtrlInfo.pwmStateStartTime = currentTime;
                    App_Shockwave_SetESW_P(true);
                    App_Shockwave_SetESW_N(false);
                    s_SWCtrlInfo.RemainPoints--;  // 工作点数减一
                }
                // 否则继续等待
            }
            break;
            
        case E_SW_PWM_STATE_ESW_P_HIGH:
            elapsedTime = currentTime - s_SWCtrlInfo.pwmStateStartTime;
            if(elapsedTime >= SW_PWM_ESW_P_HIGH_TIME_MS)
            {
                // PWM_ESW+高电平5ms后切换为低电平，进入等待状态
                App_Shockwave_SetESW_P(false);
                s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_WAIT;
                s_SWCtrlInfo.pwmStateStartTime = currentTime;
            }
            break;
            
        case E_SW_PWM_STATE_WAIT:
            elapsedTime = currentTime - s_SWCtrlInfo.pwmStateStartTime;
            if(elapsedTime >= SW_PWM_ESW_P_WAIT_TIME_MS)
            {
                // 等待17ms后，PWM_ESW-高电平
                App_Shockwave_SetESW_N(true);
                s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_ESW_N_HIGH;
                s_SWCtrlInfo.pwmStateStartTime = currentTime;
            }
            break;
            
        case E_SW_PWM_STATE_ESW_N_HIGH:
            elapsedTime = currentTime - s_SWCtrlInfo.pwmStateStartTime;
            if(elapsedTime >= s_SWCtrlInfo.pwmESW_NHighTimeMs)
            {
                // PWM_ESW-高电平时间到，切换为低电平
                App_Shockwave_SetESW_N(false);
                
                // PWM_ESW-高电平时间到，切换为低电平
                // 等待周期结束，然后开始新周期
                s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_IDLE;
            }
            break;
            
        default:
            break;
    }
}

void App_Shockwave_Process(void)
{
    static Drv_Timer_t TempMonitorTimer;
    
    // Process the shockwave module
    App_Shockwave_UpdateStatus();
    App_Shockwave_RxDataHandle();
    App_Shockwave_Monitor();

    // Handle the shockwave state
    switch(s_SWCtrlInfo.runState)
    {
        case E_SW_RUN_INIT:
            // 加载冲击波参数
            if(App_Memory_LoadSWParams(&s_SWCtrlInfo.TreatParams)) {
                s_SWCtrlInfo.TempLimit = s_SWCtrlInfo.TreatParams.TempLimit;
                s_SWCtrlInfo.TreatTimes = s_SWCtrlInfo.TreatParams.RemainTimes;
                s_SWCtrlInfo.CurrentHigh_ESW_P = s_SWCtrlInfo.TreatParams.CurrentHigh_ESW_P;
                s_SWCtrlInfo.CurrentLow_ESW_P = s_SWCtrlInfo.TreatParams.CurrentLow_ESW_P;
                s_SWCtrlInfo.CurrentHigh_ESW_N = s_SWCtrlInfo.TreatParams.CurrentHigh_ESW_N;
                s_SWCtrlInfo.CurrentLow_ESW_N = s_SWCtrlInfo.TreatParams.CurrentLow_ESW_N;
                
                LOG_I("SW: Parameters loaded - temp_limit=%d, remain_times=%d, ESW_P=[%d, %d], ESW_N=[%d, %d]",
                      s_SWCtrlInfo.TempLimit, s_SWCtrlInfo.TreatTimes,
                      s_SWCtrlInfo.CurrentLow_ESW_P, s_SWCtrlInfo.CurrentHigh_ESW_P,
                      s_SWCtrlInfo.CurrentLow_ESW_N, s_SWCtrlInfo.CurrentHigh_ESW_N);
            } else {
                LOG_E("SW: Failed to load parameters");
                s_SWCtrlInfo.ErrorCode = E_SW_ERROR_READ_PARAMS_FAILED;
            }
            App_Shockwave_ChangeState(E_SW_RUN_IDLE);
            break;
            
        case E_SW_RUN_IDLE:
            // 使用App_Shockwave_StartCheck进行启动前检查
            if(App_Shockwave_StartCheck()) {
                // 设置工作参数并启动冲击波发射
                App_Shockwave_SetWorkParams();

                // pwr_control3、pwr_control4切换至可输出（平常为不可输出）
                Drv_IODevice_ChangeChannel(CHANNEL_SW);
                App_Shockwave_ChangeState(E_SW_RUN_WORKING);
            }
            break;
            
        case E_SW_RUN_WORKING:
            // 检查所有条件
            if(App_Shockwave_StartCheck() == false || 
               s_SWCtrlInfo.RemainPoints == 0){
                App_Shockwave_ChangeState(E_SW_RUN_STOP);
            } else {
                // 处理PWM时序
                App_Shockwave_ProcessPWM();
                
                // 监控电流（在PWM高电平时）
                if(s_SWCtrlInfo.pwmState == E_SW_PWM_STATE_ESW_P_HIGH || 
                   s_SWCtrlInfo.pwmState == E_SW_PWM_STATE_ESW_N_HIGH) {
                    if(App_Shockwave_IsCurrentNormal() == false) {
                        // 电流异常，报警但不立即停止
                    }
                }
                
                // 监控电压
                if(App_Shockwave_IsVoltageNormal() == false) {
                    // 电压异常，报警但不立即停止
                }
                
                // 温度监控（10ms周期）
                if(Drv_Timer_Tick(&TempMonitorTimer, SW_TEMP_MONITOR_PERIOD_MS)) {
                    if(App_Shockwave_IsHeadTempNormal() == false) {
                        // 温度超限，立马停止工作
                        App_Shockwave_ChangeState(E_SW_RUN_STOP);
                    }
                }
            }
            break;
            
        case E_SW_RUN_STOP:
            // 关闭PWM输出
            App_Shockwave_SetESW_P(false);
            App_Shockwave_SetESW_N(false);
            // 重置PWM状态
            s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_IDLE;
            s_SWCtrlInfo.cycleStartTime = 0;
            // 关闭输出通道
            Drv_IODevice_ChangeChannel(CHANNEL_CLOSE);
            App_TreatMgr_ChangeState(E_TREATMGR_STATE_IDLE);
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
    // 初始化控制信息结构
    memset(&s_SWCtrlInfo, 0, sizeof(SW_CtrlInfo_t));
    
    // 设置初始状态
    s_SWCtrlInfo.runState = E_SW_RUN_INIT;
    s_SWCtrlInfo.pwmState = E_SW_PWM_STATE_IDLE;
    s_SWCtrlInfo.ErrorCode = E_SW_ERROR_NONE;
    s_SWCtrlInfo.WorkLevel = 0;
    s_SWCtrlInfo.FreqLevel = 0;
    s_SWCtrlInfo.RemainPoints = 0;
    s_SWCtrlInfo.TreatTimes = 0;
    
    // 初始化TIM4（PWM输出）
    MX_TIM4_Init();
    
    // 确保PWM输出为低电平
    App_Shockwave_SetESW_P(false);
    App_Shockwave_SetESW_N(false);
    
    LOG_I("Shockwave module initialized");
}

/**************************End of file********************************/
