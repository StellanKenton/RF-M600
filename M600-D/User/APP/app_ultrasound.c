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
        
        // 处理复位功能
        if(s_USCtrlInfo.Trans.RxWorkState.work_state == WORK_STATE_RESET)
        {
            // 重置治疗时间和治疗档位
            s_USCtrlInfo.RemainTime = s_USCtrlInfo.Trans.RxWorkState.work_time;
            s_USCtrlInfo.WorkLevel = s_USCtrlInfo.Trans.RxWorkState.work_level;
            
            // 剩余可治疗次数减一
            if(s_USCtrlInfo.TreatTimes > 0)
            {
                s_USCtrlInfo.TreatTimes--;
                // 保存到存储器
                s_USCtrlInfo.TreatParams.RemainTimes = s_USCtrlInfo.TreatTimes;
                App_Memory_SaveUSParams(&s_USCtrlInfo.TreatParams);
                LOG_I("Reset: Remaining treat times decreased to: %d", s_USCtrlInfo.TreatTimes);
            }
            
            LOG_I("Reset: Work time=%d, Work level=%d", s_USCtrlInfo.RemainTime, s_USCtrlInfo.WorkLevel);
        }
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
                // 启动工作时蜂鸣器提示（2s）
                Drv_IODevice_StartBuzzer(2000);
                break;
            case E_US_RUN_STOP:
                LOG_I("Ultrasound state changed to STOP");
                // 结束工作时蜂鸣器提示（2s）
                Drv_IODevice_StartBuzzer(2000);
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
    
    // 计算脉冲重复时间：档位0=20ms，档位39=0.5ms
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
    
    // 转换为微秒并设置到SI5351 (输入单位是微秒，0.5ms = 500us)
    uint16_t pulse_time_us = (uint16_t)(pulse_time_ms * 1000);
    Drv_SI5351_SetPulseWidthus(pulse_time_us);
    s_USCtrlInfo.WorkLevel = level;
    LOG_I("Ultrasound level set to: %d (pulse time: %.1f ms)", level, pulse_time_ms);
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
    
    // 3. 检查工作档位是否不等于0（0-40）
    if(s_USCtrlInfo.Trans.RxWorkState.work_level == 0 || s_USCtrlInfo.Trans.RxWorkState.work_level > WORK_LEVEL_MAX) {
        LOG_E("Invalid work level: %d (range: 1-%d)", s_USCtrlInfo.Trans.RxWorkState.work_level, WORK_LEVEL_MAX);
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
    s_USCtrlInfo.VoltageBase = s_USCtrlInfo.Trans.RxConfig.voltage;  // 保存基础电压用于超限检测
    s_USCtrlInfo.CurrentHigh = s_USCtrlInfo.TreatParams.CurrentHigh;
    s_USCtrlInfo.CurrentLow = s_USCtrlInfo.TreatParams.CurrentLow;
    s_USCtrlInfo.Frequency = s_USCtrlInfo.Trans.RxConfig.frequency;
    s_USCtrlInfo.TempLimit = s_USCtrlInfo.Trans.RxConfig.temp_limit;
    
    // 剩余可治疗次数减一
    if(s_USCtrlInfo.TreatTimes > 0)
    {
        s_USCtrlInfo.TreatTimes--;
        // 保存到存储器
        s_USCtrlInfo.TreatParams.RemainTimes = s_USCtrlInfo.TreatTimes;
        App_Memory_SaveUSParams(&s_USCtrlInfo.TreatParams);
        LOG_I("Remaining treat times decreased to: %d", s_USCtrlInfo.TreatTimes);
    }
    
    // 配置工作电压和工作频率
    App_Ultrasound_SetFrequency(s_USCtrlInfo.Frequency);
    App_UltraSound_SetLevel(s_USCtrlInfo.WorkLevel);
    
    // 设置初始工作电压
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
        // 电流过高，需要降低电压
        // 简单的PI控制：根据电流偏差调整电压
        int16_t currentError = current - ((s_USCtrlInfo.CurrentHigh + s_USCtrlInfo.CurrentLow) / 2);
        voltageAdjust = -(currentError * 10) / 100;  // 简单的比例控制
        
        s_USCtrlInfo.ErrorCode = E_US_ERROR_CURRENT_TOO_HIGH;
        LOG_W("Current is too high: %d (target: %d-%d)", current, s_USCtrlInfo.CurrentLow, s_USCtrlInfo.CurrentHigh);
    }
    else if(current < s_USCtrlInfo.CurrentLow)
    {
        // 电流过低，需要提高电压
        int16_t currentError = ((s_USCtrlInfo.CurrentHigh + s_USCtrlInfo.CurrentLow) / 2) - current;
        voltageAdjust = (currentError * 10) / 100;  // 简单的比例控制
        
        s_USCtrlInfo.ErrorCode = E_US_ERROR_CURRENT_TOO_LOW;
        LOG_W("Current is too low: %d (target: %d-%d)", current, s_USCtrlInfo.CurrentLow, s_USCtrlInfo.CurrentHigh);
    }
    else
    {
        s_USCtrlInfo.ErrorCode = E_US_ERROR_NONE;
    }
    
    // 如果需要进行电压调节
    if(voltageAdjust != 0)
    {
        newVoltage = currentVoltage + voltageAdjust;
        
        // 检查电压调节是否超过限制（±2V）
        int16_t voltageDiff = (int16_t)newVoltage - (int16_t)s_USCtrlInfo.VoltageBase;
        if(voltageDiff > VOLTAGE_ADJUST_LIMIT_MV || voltageDiff < -VOLTAGE_ADJUST_LIMIT_MV)
        {
            // 电压超限，报警
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
            
            // 设置新电压
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
        
        // 温度超限，自动降低档位（可低至0档）
        if(s_USCtrlInfo.WorkLevel > 0)
        {
            s_USCtrlInfo.WorkLevel--;
            App_UltraSound_SetLevel(s_USCtrlInfo.WorkLevel);
            LOG_W("Auto reduce level to: %d", s_USCtrlInfo.WorkLevel);
        }
        else
        {
            // 已经是最低档位，停止工作
            isNormal = false;
        }
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

                // pwr_control2切换至可输出（平常为不可输出）
                Drv_IODevice_ChangeChannel(CHANNEL_US);
                App_Ultrasound_ChangeState(E_US_RUN_WORKING);
            }
            break;
        case E_US_RUN_WORKING:
            // 检查所有条件
            if(App_UltraSound_StartCheck() == false || 
            App_UltraSound_IsCurrentNormal() == false || 
            App_UltraSound_IsHeadTempNormal() == false ||
            s_USCtrlInfo.RemainTime == 0){
                App_Ultrasound_ChangeState(E_US_RUN_STOP);
            } else {
                if(s_USCtrlInfo.RemainTime >= TREAT_TASK_TIME) {
                    s_USCtrlInfo.RemainTime -= TREAT_TASK_TIME;
                } else {
                    s_USCtrlInfo.RemainTime = 0;
                }
            }
            break;
        case E_US_RUN_STOP:
            // 关闭输出通道
            Drv_IODevice_ChangeChannel(CHANNEL_CLOSE);
            // 停止DAC输出
            Drv_DAC_SetVoltage(0);
            App_TreatMgr_ChangeState(E_TREATMGR_STATE_IDLE);
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
    // 初始化控制信息结构
    memset(&s_USCtrlInfo, 0, sizeof(US_CtrlInfo_t));
    
    // 设置初始状态
    s_USCtrlInfo.runState = E_US_RUN_INIT;
    s_USCtrlInfo.ErrorCode = E_US_ERROR_NONE;
    s_USCtrlInfo.WorkLevel = 0;
    s_USCtrlInfo.RemainTime = 0;
    s_USCtrlInfo.TreatTimes = 0;
    
    // 初始化DAC
    Drv_DAC_Init();
    
    // 初始化SI5351
    Drv_SI5351_Init();
    
    LOG_I("Ultrasound module initialized");
}

/**************************End of file********************************/
