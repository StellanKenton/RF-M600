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
#include "drv_dac.h"
#include "drv_adc.h"
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
    s_USCtrlInfo.Trans.TxStatus.remain_time = s_USCtrlInfo.TreatCounts/1000;
    s_USCtrlInfo.Trans.TxStatus.work_level = s_USCtrlInfo.WorkLevel;
    s_USCtrlInfo.Trans.TxStatus.head_temp = s_USCtrlInfo.HeadTemp;
    // 探头与脚踏状态由 mgr 刷新，conn_state 由 comm 按 mgr 状态上传；此处仅保留协议字段
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
                // �?动工作时蜂鸣器提示（2s�?
                Drv_IODevice_StartBuzzer(2000);
                break;
            case E_US_RUN_STOP:
                LOG_I("Ultrasound state changed to STOP");
                // 结束工作时蜂鸣器提示�?2s�?
                Drv_IODevice_StartBuzzer(2000);
                break;
            default:
                break;
        }
    }
}

void App_Ultrasound_Monitor(void)
{
    static uint8_t s_lastRxWorkState = 0x00;
    if(s_USCtrlInfo.Trans.RxWorkState.work_state != s_lastRxWorkState) {
        /* 当 work_state 从其他状态跳转到 0x02(Reset) 时，进入 E_TREAT_TIMES_RESET */
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
    // 设置频率到SI5351
    frequency = Drv_SI5351_SetFrequency(frequency);
    s_USCtrlInfo.Frequency = frequency;
    LOG_I("Ultrasound frequency set to: %d kHz", frequency);
}

void App_UltraSound_SetLevel(uint8_t level)
{
    if(level > WORK_LEVEL_MAX)
    {
        LOG_E("Invalid level: %d (range: 0-%d)", level, WORK_LEVEL_MAX);
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        return;
    }
    
    // 计算脉冲重�?�时间：档位0=20ms，档�?39=0.5ms
    // pulse_time = 20ms - level * 0.5ms
    float pulse_time_ms = PULSE_REPEAT_TIME_BASE_MS - (level * PULSE_REPEAT_TIME_STEP_MS);
    
    // 限制范围
    if(pulse_time_ms < PULSE_REPEAT_TIME_MIN_MS)
    {
        pulse_time_ms = PULSE_REPEAT_TIME_MIN_MS;
    }
    else if(pulse_time_ms > PULSE_REPEAT_TIME_MAX_MS)
    {
        pulse_time_ms = PULSE_REPEAT_TIME_MAX_MS;
    }
    
    // �?�?为微秒并设置到SI5351 (输入单位�?�?秒，0.5ms = 500us)
    uint16_t pulse_time_us = (uint16_t)(pulse_time_ms * 1000);
    Drv_SI5351_SetPulseWidthus(pulse_time_us);
    s_USCtrlInfo.WorkLevel = level;
    LOG_I("Ultrasound level set to: %d (pulse time: %.1f ms)", level, pulse_time_ms);
}


bool App_UltraSound_StartCheck()
{
    // 1. 检查下位机�?否下发了发射超声指令
    if(s_USCtrlInfo.Trans.RxWorkState.work_state != 0x01) {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        s_USCtrlInfo.StartCheckStep = 1;
        return false;
    }
    
    // 2. 检查剩余工作时间是否大�?0�?0-3600s�?
    if(s_USCtrlInfo.Trans.RxWorkState.work_time == 0 || s_USCtrlInfo.Trans.RxWorkState.work_time > 3600) {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        s_USCtrlInfo.StartCheckStep = 2;
        return false;
    }
    
    // 3. 检查工作档位是否不等于0�?0-40�?
    if(s_USCtrlInfo.Trans.RxWorkState.work_level == 0 || s_USCtrlInfo.Trans.RxWorkState.work_level > WORK_LEVEL_MAX) {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        s_USCtrlInfo.StartCheckStep = 3;
        return false;
    }
    
    // 4. 检查脚踏开关是否闭�?
    if(!App_TreatMgr_GetFootSwitchClosed()) {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        s_USCtrlInfo.StartCheckStep = 4;
        return false;
    }
    
    // 5. 检查是否�?�确识别到超声治疗头
    if(App_TreatMgr_GetProbeStatus() != E_IODEVICE_MODE_ULTRASOUND) {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_PROBE_NOT_CONNECTED;
        s_USCtrlInfo.StartCheckStep = 5;
        return false;
    }
    
    // 6. 检查是否有剩余�?治疗次数
    if(s_USCtrlInfo.TreatCounts == 0) {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        s_USCtrlInfo.StartCheckStep = 6;
        return false;
    }
    
    // 7. 检查配�?参数�?否有�?
    if(s_USCtrlInfo.Trans.RxConfig.frequency == 0 || s_USCtrlInfo.Trans.RxConfig.temp_limit == 0 || s_USCtrlInfo.Trans.RxConfig.voltage == 0) {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        s_USCtrlInfo.StartCheckStep = 7;
        return false;
    }
    
    // 8. 检查治疗参数是否有�?
    if(s_USCtrlInfo.CurrentHigh == 0) {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_INVALID_PARAMS;
        s_USCtrlInfo.StartCheckStep = 8;
        return false;
    }
    
    // 所有�?�查通过
    LOG_I("US: Start check passed");
    return true;
}

void App_UltraSound_SetWorkParams(void)
{
    // 设置工作参数
    s_USCtrlInfo.WorkLevel = s_USCtrlInfo.Trans.RxWorkState.work_level;
    s_USCtrlInfo.Voltage = s_USCtrlInfo.Trans.RxConfig.voltage;
    s_USCtrlInfo.VoltageBase = s_USCtrlInfo.Trans.RxConfig.voltage;  // 保存基�?�电压用于超限检�?
    s_USCtrlInfo.Frequency = s_USCtrlInfo.Trans.RxConfig.frequency;
    s_USCtrlInfo.TempLimit = s_USCtrlInfo.Trans.RxConfig.temp_limit;
    
    // 配置工作电压和工作�?�率
    App_Ultrasound_SetFrequency(s_USCtrlInfo.Frequency);
    App_UltraSound_SetLevel(s_USCtrlInfo.WorkLevel);
    
    // 设置初�?�工作电�?
    Drv_DAC_SetVoltage(s_USCtrlInfo.Voltage);
    
    // 切换继电器pwr_control1至超声通道
    Drv_IODevice_ChangeChannel(CHANNEL_READY);
}

bool App_UltraSound_IsCurrentNormal(void)
{
    bool isNormal = true;
    uint16_t current = Drv_ADC_GetRealValue(E_ADC_CHANNEL_US_I);
    uint16_t currentVoltage = Drv_DAC_GetVoltage();
    int16_t voltageAdjust = 0;
    uint16_t newVoltage = currentVoltage;
    
    if(current > s_USCtrlInfo.CurrentHigh)
    {
        // 电流过高，需要降低电�?
        // 简单的PI控制：根�?电流偏差调整电压
        int16_t currentError = current - ((s_USCtrlInfo.CurrentHigh + s_USCtrlInfo.CurrentLow) / 2);
        voltageAdjust = -(currentError * 10) / 100;  // 简单的比例控制
        
        s_USCtrlInfo.ErrorCode = E_US_ERROR_CURRENT_TOO_HIGH;
        LOG_W("Current is too high: %d (target: %d-%d)", current, s_USCtrlInfo.CurrentLow, s_USCtrlInfo.CurrentHigh);
    }
    else if(current < s_USCtrlInfo.CurrentLow)
    {
        // 电流过低，需要提高电�?
        int16_t currentError = ((s_USCtrlInfo.CurrentHigh + s_USCtrlInfo.CurrentLow) / 2) - current;
        voltageAdjust = (currentError * 10) / 100;  // 简单的比例控制
        
        s_USCtrlInfo.ErrorCode = E_US_ERROR_CURRENT_TOO_LOW;
        LOG_W("Current is too low: %d (target: %d-%d)", current, s_USCtrlInfo.CurrentLow, s_USCtrlInfo.CurrentHigh);
    }
    else
    {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_NONE;
    }
    
    // 如果需要进行电压调�?
    if(voltageAdjust != 0)
    {
        newVoltage = currentVoltage + voltageAdjust;
        
        // 检查电压调节是否超过限制（±2V�?
        int16_t voltageDiff = (int16_t)newVoltage - (int16_t)s_USCtrlInfo.VoltageBase;
        if(voltageDiff > VOLTAGE_ADJUST_LIMIT_MV || voltageDiff < -VOLTAGE_ADJUST_LIMIT_MV)
        {
            // 电压超限，报�?
            s_USCtrlInfo.ErrorCode = E_US_ERROR_VOLTAGE_OVER_LIMIT;
            LOG_E("Voltage adjust over limit: %d mV (base: %d mV, limit: ±%d mV)", 
                  newVoltage, s_USCtrlInfo.VoltageBase, VOLTAGE_ADJUST_LIMIT_MV);
            isNormal = false;
        }
        else
        {
            // 限制电压范围
            if(newVoltage > 3300)
            {
                newVoltage = 3300;
            }
            else if(newVoltage <= 0)
            {
                newVoltage = 0;
            }
            
            // 设置新电�?
            Drv_DAC_SetVoltage(newVoltage);
            LOG_I("Voltage adjusted: %d -> %d mV (current: %d)", currentVoltage, newVoltage, current);
        }
    }
    
    return isNormal;
}

bool App_UltraSound_IsHeadTempNormal(void)
{
    bool isNormal = true;
    uint16_t temp = Drv_ADC_GetRealValue(E_ADC_CHANNEL_HAND_NTC);
    s_USCtrlInfo.HeadTemp = temp;
    
    if(temp > s_USCtrlInfo.TempLimit)
    {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_TEMP_TOO_HIGH;
        LOG_W("Head temperature too high: %d (limit: %d)", temp, s_USCtrlInfo.TempLimit);
        
        // 温度超限，自动降低档位（�?低至0档）
        if(s_USCtrlInfo.WorkLevel > 0)
        {
            s_USCtrlInfo.WorkLevel--;
            App_UltraSound_SetLevel(s_USCtrlInfo.WorkLevel);
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
    static uint16_t debounceCount = 0;
    if(App_TreatMgr_GetProbeStatus() != E_IODEVICE_MODE_ULTRASOUND) {
        debounceCount++;
        if(debounceCount >= PROBE_STATUS_DEBOUNCE_CNT) {
            debounceCount = 0;
            s_USCtrlInfo.isWaitReturn = true;
        }
    } else {
        debounceCount = 0;
    }

    if(s_USCtrlInfo.isWaitReturn) {
        App_Ultrasound_ChangeState(E_US_RUN_STOP);
    }
}

void App_Ultrasound_Process(void)
{
    // Process the ultrasound module
    App_UltraSound_UpdateStatus();
    App_UltraSound_RxDataHandle();
    App_Ultrasound_Monitor();
    App_Ultrasound_CheckProbe();
    // Handle the ultrasound state
    switch(s_USCtrlInfo.runState)
    {
        case E_US_RUN_INIT:
            if(App_Memory_LoadUSParams(&s_USCtrlInfo.TreatParams)) {
                // 加载治疗参数
                s_USCtrlInfo.Trans.RxConfig.frequency = s_USCtrlInfo.TreatParams.Frequency;
                s_USCtrlInfo.Trans.RxConfig.temp_limit = s_USCtrlInfo.TreatParams.TempLimit;
                s_USCtrlInfo.Trans.RxConfig.voltage = s_USCtrlInfo.TreatParams.Voltage;

                s_USCtrlInfo.TreatRemainTimes = s_USCtrlInfo.TreatParams.TreatRemainTimes;
                s_USCtrlInfo.CurrentHigh = s_USCtrlInfo.TreatParams.CurrentHigh;
                s_USCtrlInfo.CurrentLow = s_USCtrlInfo.TreatParams.CurrentLow;
                s_USCtrlInfo.VoltageBase = s_USCtrlInfo.TreatParams.Voltage;
                
            } else {
                LOG_E("Failed to load ultrasound parameters");
                s_USCtrlInfo.ErrorCode = E_US_ERROR_READ_PARAMS_FAILED;
                s_USCtrlInfo.Trans.RxConfig.frequency = 1200;
                s_USCtrlInfo.Trans.RxConfig.temp_limit = 40;
                s_USCtrlInfo.Trans.RxConfig.voltage = 1500;
                s_USCtrlInfo.TreatRemainTimes = 0;

                s_USCtrlInfo.CurrentHigh = 1000;    // default value
                s_USCtrlInfo.CurrentLow = 500;
                s_USCtrlInfo.VoltageBase = 1000;
            }
			
            App_Ultrasound_ChangeState(E_US_RUN_IDLE);
            break;
        case E_US_RUN_IDLE:
            // 使用App_UltraSound_StartCheck进�?�启动前检查（包含所有参数�?�查）
            if(App_UltraSound_StartCheck()) {
                // 设置工作参数并启动超声发�?
                App_UltraSound_SetWorkParams();

                // pwr_control2切换至可输出（平常为不可输出�?
                Drv_IODevice_ChangeChannel(CHANNEL_US);
                App_Ultrasound_ChangeState(E_US_RUN_WORKING);
            }
            break;
        case E_US_RUN_WORKING:           
            // 检查所有条�?
            if(App_UltraSound_StartCheck() == false || 
            App_UltraSound_IsCurrentNormal() == false || 
            App_UltraSound_IsHeadTempNormal() == false ){
                App_Ultrasound_ChangeState(E_US_RUN_STOP);
            }
            break;
        case E_US_RUN_STOP:
            // 关闭输出通道
            Drv_IODevice_ChangeChannel(CHANNEL_CLOSE);
            // 停�??DAC输出
            Drv_DAC_SetVoltage(0);
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
    // 初�?�化控制信息结构
    memset(&s_USCtrlInfo, 0, sizeof(US_CtrlInfo_t));
    
    // 设置初�?�状�?
    s_USCtrlInfo.runState = E_US_RUN_INIT;
    s_USCtrlInfo.ErrorCode = E_US_ERROR_NONE;
    s_USCtrlInfo.WorkLevel = 0;
    s_USCtrlInfo.TreatCounts = 0;
    
    // 初�?�化DAC
    Drv_DAC_Init();
    
    // 初�?�化SI5351
    Drv_SI5351_Init();
    
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

/**************************End of file********************************/
