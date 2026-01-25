/***********************************************************************************
* @file     : app_radiofreq.c
* @brief    : Radio Frequency treatment module implementation
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "app_radiofreq.h"
#include "app_treatmgr.h"
#include "app_memory.h"
#include "app_comm.h"
#include "drv_iodevice.h"
#include "drv_dac.h"
#include "drv_adc.h"
#include "log.h"
#include "drv_si5351.h"
#include "drv_delay.h"
#include <string.h>

static RF_CtrlInfo_t s_RFCtrlInfo;

/**
 * @brief Calculate voltage from work level (1-20档位对应11-30V)
 * @param level Work level (0-20)
 * @retval Voltage in mV
 */
static uint16_t App_RadioFreq_CalculateVoltage(uint8_t level)
{
    if(level == 0) {
        return RF_VOLTAGE_INIT_MV;  // 档位0对应7V
    }
    if(level > RF_WORK_LEVEL_MAX) {
        level = RF_WORK_LEVEL_MAX;
    }
    // 1档对应11V，20档对应30V
    // voltage = 11000 + (level - 1) * (30000 - 11000) / (20 - 1)
    return RF_VOLTAGE_MIN_MV + ((level - 1) * (RF_VOLTAGE_MAX_MV - RF_VOLTAGE_MIN_MV)) / (RF_WORK_LEVEL_MAX - 1);
}

void App_RadioFreq_UpdateStatus(void)
{
    // Update work state
    if(s_RFCtrlInfo.runState == E_RF_RUN_WORKING) {
        s_RFCtrlInfo.Trans.TxStatus.work_state = 0x01;
    } else if(s_RFCtrlInfo.runState == E_RF_RUN_STOP) {
        s_RFCtrlInfo.Trans.TxStatus.work_state = 0x00;
    }
    s_RFCtrlInfo.Trans.TxStatus.temp_limit = s_RFCtrlInfo.TempLimit;
    s_RFCtrlInfo.Trans.TxStatus.remain_time = s_RFCtrlInfo.RemainTime;
    s_RFCtrlInfo.Trans.TxStatus.work_level = s_RFCtrlInfo.WorkLevel;
    s_RFCtrlInfo.Trans.TxStatus.head_temp = s_RFCtrlInfo.HeadTemp;
    
    // Get probe connection status and foot switch state
    bool headConnected = (s_RFCtrlInfo.probeStatus == E_IODEVICE_MODE_RADIO_FREQUENCY);
    
    // Combine connection state: bit[4]=head connection, bit[0]=foot switch
    if (headConnected && s_RFCtrlInfo.FootSwitchStatus) {
        s_RFCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_CONNECTED_FOOT_CLOSED;
    } else if (!headConnected && s_RFCtrlInfo.FootSwitchStatus) {
        s_RFCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_DISCONNECTED_FOOT_CLOSED;
    } else if (headConnected && !s_RFCtrlInfo.FootSwitchStatus) {
        s_RFCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_CONNECTED_FOOT_OPEN;
    } else {
        s_RFCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_DISCONNECTED_FOOT_OPEN;
    }
    s_RFCtrlInfo.Trans.TxStatus.error_code = s_RFCtrlInfo.ErrorCode;
}

void App_RadioFreq_RxDataHandle(void)
{
    RF_TransData_t *pTransData = App_Comm_GetRFTransData();
    
    if(pTransData->RxWorkState.work_state == WORK_STATE_RESET)
    {
        // 处理复位功能
        // 重置治疗时间和治疗档位
        s_RFCtrlInfo.RemainTime = pTransData->RxWorkState.work_time;
        s_RFCtrlInfo.WorkLevel = pTransData->RxWorkState.work_level;
        
        // 剩余可治疗次数减一
        if(s_RFCtrlInfo.TreatTimes > 0)
        {
            s_RFCtrlInfo.TreatTimes--;
            // 保存到存储器
            s_RFCtrlInfo.TreatParams.RemainTimes = s_RFCtrlInfo.TreatTimes;
            App_Memory_SaveRFParams(&s_RFCtrlInfo.TreatParams);
            LOG_I("RF Reset: Remaining treat times decreased to: %d", s_RFCtrlInfo.TreatTimes);
        }
        
        LOG_I("RF Reset: Work time=%d, Work level=%d", s_RFCtrlInfo.RemainTime, s_RFCtrlInfo.WorkLevel);
    }
    
    // 处理配置更新
    if(pTransData->flag.bits.Rely_Config)
    {
        s_RFCtrlInfo.TempLimit = pTransData->RxConfig.temp_limit;
        // 保存到存储器
        s_RFCtrlInfo.TreatParams.TempLimit = s_RFCtrlInfo.TempLimit;
        App_Memory_SaveRFParams(&s_RFCtrlInfo.TreatParams);
        pTransData->flag.bits.Rely_Config = 0;
        LOG_I("RF Config updated: temp_limit=%d", s_RFCtrlInfo.TempLimit);
    }
}

void App_RadioFreq_ChangeState(RF_RunState_EnumDef newState)
{
    if(newState != s_RFCtrlInfo.runState && newState < E_RF_RUN_MAX)
    {
        s_RFCtrlInfo.runState = newState;
        switch(newState)
        {
            case E_RF_RUN_INIT:
                LOG_I("RF state changed to INIT");
                break;
            case E_RF_RUN_IDLE:
                LOG_I("RF state changed to IDLE");
                break;
            case E_RF_RUN_WORKING:
                LOG_I("RF state changed to WORKING");
                // 启动工作时蜂鸣器提示（2s）
                Drv_IODevice_StartBuzzer(2000);
                break;
            case E_RF_RUN_STOP:
                LOG_I("RF state changed to STOP");
                // 结束工作时蜂鸣器提示（2s）
                Drv_IODevice_StartBuzzer(2000);
                break;
            default:
                break;
        }
    }
}

void App_RadioFreq_Monitor(void)
{
    // Get the foot switch status and probe status
    s_RFCtrlInfo.FootSwitchStatus = Drv_IODevice_GetFootSwitchState();
    
    // Get the probe status
    s_RFCtrlInfo.probeStatus = Drv_IODevice_GetProbeStatus();
}

bool App_RadioFreq_StartCheck()
{
    RF_TransData_t *pTransData = App_Comm_GetRFTransData();
    
    // 1. 检查下位机是否下发了发射射频指令
    if(pTransData->RxWorkState.work_state != WORK_STATE_START) {
        LOG_E("RF: Work state is not start");
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 2. 检查剩余工作时间是否大于0（0-3600s）
    if(pTransData->RxWorkState.work_time == 0 || pTransData->RxWorkState.work_time > 3600) {
        LOG_E("RF: Invalid work time: %d", pTransData->RxWorkState.work_time);
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 3. 检查工作档位是否不等于0（0-20）
    if(pTransData->RxWorkState.work_level == 0 || pTransData->RxWorkState.work_level > RF_WORK_LEVEL_MAX) {
        LOG_E("RF: Invalid work level: %d (range: 1-%d)", pTransData->RxWorkState.work_level, RF_WORK_LEVEL_MAX);
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 4. 检查脚踏开关是否闭合
    if(s_RFCtrlInfo.FootSwitchStatus == false) {
        LOG_E("RF: Foot switch is not closed");
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 5. 检查是否正确识别到射频治疗头
    if(s_RFCtrlInfo.probeStatus != E_IODEVICE_MODE_RADIO_FREQUENCY) {
        LOG_E("RF: Radio frequency probe not connected");
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_PROBE_NOT_CONNECTED;
        return false;
    }
    
    // 6. 检查是否有剩余可治疗次数
    if(s_RFCtrlInfo.TreatTimes == 0) {
        LOG_E("RF: No remaining treat times");
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 所有检查通过
    return true;
}

void App_RadioFreq_SetWorkParams(void)
{
    RF_TransData_t *pTransData = App_Comm_GetRFTransData();
    
    // 设置工作参数
    s_RFCtrlInfo.WorkLevel = pTransData->RxWorkState.work_level;
    s_RFCtrlInfo.RemainTime = pTransData->RxWorkState.work_time;
    
    // 计算目标工作电压（根据档位）
    s_RFCtrlInfo.VoltageTarget = App_RadioFreq_CalculateVoltage(s_RFCtrlInfo.WorkLevel);
    
    // 设置初始工作电压为7V
    s_RFCtrlInfo.Voltage = RF_VOLTAGE_INIT_MV;
    Drv_DAC_SetVoltage(s_RFCtrlInfo.Voltage);
    
    // 配置SI5351输出1MHz互补PWM（带死区时间）
    // 死区时间根据实际需求设置，例如100ns
    Drv_SI5351_SetComplementaryPWM(RF_FREQUENCY_KHZ, 100);
    
    // 切换继电器pwr_control1至射频通道
    Drv_IODevice_ChangeChannel(CHANNEL_READY);
    
    // CTR_HEAT_HP为高电平为治疗头检测板供电
    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 1);
    
    LOG_I("RF: Work params set - level=%d, time=%d, voltage_target=%d", 
          s_RFCtrlInfo.WorkLevel, s_RFCtrlInfo.RemainTime, s_RFCtrlInfo.VoltageTarget);
}

bool App_RadioFreq_IsCurrentNormal(void)
{
    uint16_t current = Drv_ADC_GetRealValue(E_ADC_CHANNEL_RF_I);
    uint16_t currentVoltage = Drv_DAC_GetVoltage();
    uint16_t newVoltage = currentVoltage;
    bool isNormal = true;
    
    if(current < RF_CURRENT_THRESHOLD_MV)
    {
        // 射频电流低于0.5V，工作电压维持在7V
        if(currentVoltage != RF_VOLTAGE_INIT_MV)
        {
            newVoltage = RF_VOLTAGE_INIT_MV;
            Drv_DAC_SetVoltage(newVoltage);
            s_RFCtrlInfo.Voltage = newVoltage;
            LOG_I("RF: Current too low (%d mV), voltage set to 7V", current);
        }
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_CURRENT_TOO_LOW;
        isNormal = false;
    }
    else if(current >= s_RFCtrlInfo.CurrentLow)
    {
        // 射频电流大于工作电流下限，电压切换至档位对应的工作电压
        if(currentVoltage != s_RFCtrlInfo.VoltageTarget)
        {
            newVoltage = s_RFCtrlInfo.VoltageTarget;
            Drv_DAC_SetVoltage(newVoltage);
            s_RFCtrlInfo.Voltage = newVoltage;
            LOG_I("RF: Current normal (%d mV), voltage set to %d mV (level %d)", 
                  current, newVoltage, s_RFCtrlInfo.WorkLevel);
        }
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_NONE;
    }
    else
    {
        // 电流在工作电流区间以下，自动切换至7V
        if(currentVoltage != RF_VOLTAGE_INIT_MV)
        {
            newVoltage = RF_VOLTAGE_INIT_MV;
            Drv_DAC_SetVoltage(newVoltage);
            s_RFCtrlInfo.Voltage = newVoltage;
            LOG_I("RF: Current below range (%d mV), voltage set to 7V", current);
        }
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_CURRENT_TOO_LOW;
        isNormal = false;
    }
    
    return isNormal;
}

bool App_RadioFreq_IsHeadTempNormal(void)
{
    // TODO: 通过治疗头串口读取温度（RF_TX/RF_RX）
    // 这里暂时使用占位符，需要根据实际串口协议实现
    // 假设从串口读取的温度值存储在s_RFCtrlInfo.HeadTemp中
    
    bool isNormal = true;
    
    if(s_RFCtrlInfo.HeadTemp > s_RFCtrlInfo.TempLimit)
    {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_TEMP_TOO_HIGH;
        LOG_W("RF: Head temperature too high: %d (limit: %d)", 
              s_RFCtrlInfo.HeadTemp, s_RFCtrlInfo.TempLimit);
        // 温度超限，自动停止输出
        isNormal = false;
    }
    else
    {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_NONE;
    }
    
    return isNormal;
}

void App_RadioFreq_Process(void)
{
    static Drv_Timer_t CurrentMonitorTimer;
    static Drv_Timer_t TempMonitorTimer;
    
    // Process the radio frequency module
    App_RadioFreq_UpdateStatus();
    App_RadioFreq_RxDataHandle();
    App_RadioFreq_Monitor();

    // Handle the radio frequency state
    switch(s_RFCtrlInfo.runState)
    {
        case E_RF_RUN_INIT:
            // 加载射频参数
            if(App_Memory_LoadRFParams(&s_RFCtrlInfo.TreatParams)) {
                s_RFCtrlInfo.TempLimit = s_RFCtrlInfo.TreatParams.TempLimit;
                s_RFCtrlInfo.TreatTimes = s_RFCtrlInfo.TreatParams.RemainTimes;
                s_RFCtrlInfo.CurrentHigh = s_RFCtrlInfo.TreatParams.CurrentHigh;
                s_RFCtrlInfo.CurrentLow = s_RFCtrlInfo.TreatParams.CurrentLow;
                
                // 更新串口配置
                RF_TransData_t *pTransData = App_Comm_GetRFTransData();
                pTransData->RxConfig.temp_limit = s_RFCtrlInfo.TempLimit;
                
                LOG_I("RF: Parameters loaded - temp_limit=%d, remain_times=%d, current_range=[%d, %d]",
                      s_RFCtrlInfo.TempLimit, s_RFCtrlInfo.TreatTimes,
                      s_RFCtrlInfo.CurrentLow, s_RFCtrlInfo.CurrentHigh);
            } else {
                LOG_E("RF: Failed to load parameters");
                s_RFCtrlInfo.ErrorCode = E_RF_ERROR_READ_PARAMS_FAILED;
            }
            App_RadioFreq_ChangeState(E_RF_RUN_IDLE);
            break;
            
        case E_RF_RUN_IDLE:
            // 使用App_RadioFreq_StartCheck进行启动前检查
            if(App_RadioFreq_StartCheck()) {
                // 设置工作参数并启动射频发射
                App_RadioFreq_SetWorkParams();

                // pwr_control2切换至可输出（平常为不可输出）
                Drv_IODevice_ChangeChannel(CHANNEL_RF);
                App_RadioFreq_ChangeState(E_RF_RUN_WORKING);
            }
            break;
            
        case E_RF_RUN_WORKING:
            // 检查所有条件
            if(App_RadioFreq_StartCheck() == false || 
               s_RFCtrlInfo.RemainTime == 0){
                App_RadioFreq_ChangeState(E_RF_RUN_STOP);
            } else {
                // 电流监控（10ms周期）
                if(Drv_Timer_Tick(&CurrentMonitorTimer, RF_CURRENT_MONITOR_PERIOD_MS)) {
                    if(App_RadioFreq_IsCurrentNormal() == false) {
                        // 电流异常，但不立即停止，继续监控
                    }
                }
                
                // 温度监控（1s周期）
                if(Drv_Timer_Tick(&TempMonitorTimer, RF_TEMP_MONITOR_PERIOD_MS)) {
                    if(App_RadioFreq_IsHeadTempNormal() == false) {
                        // 温度超限，停止输出
                        App_RadioFreq_ChangeState(E_RF_RUN_STOP);
                    }
                }
                
                // 更新时间
                if(s_RFCtrlInfo.RemainTime >= TREAT_TASK_TIME) {
                    s_RFCtrlInfo.RemainTime -= TREAT_TASK_TIME;
                } else {
                    s_RFCtrlInfo.RemainTime = 0;
                }
            }
            break;
            
        case E_RF_RUN_STOP:
            // 关闭输出通道
            Drv_IODevice_ChangeChannel(CHANNEL_CLOSE);
            // 停止DAC输出
            Drv_DAC_SetVoltage(0);
            // CTR_HEAT_HP恢复为低电平
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 0);
            App_TreatMgr_ChangeState(E_TREATMGR_STATE_IDLE);
            break;
            
        default:
            break;
    }
}

/**
 * @brief Initialize radio frequency module
 */
void App_RadioFreq_Init(void)
{
    // 初始化控制信息结构
    memset(&s_RFCtrlInfo, 0, sizeof(RF_CtrlInfo_t));
    
    // 设置初始状态
    s_RFCtrlInfo.runState = E_RF_RUN_INIT;
    s_RFCtrlInfo.ErrorCode = E_RF_ERROR_NONE;
    s_RFCtrlInfo.WorkLevel = 0;
    s_RFCtrlInfo.RemainTime = 0;
    s_RFCtrlInfo.TreatTimes = 0;
    s_RFCtrlInfo.Voltage = RF_VOLTAGE_INIT_MV;
    
    // 初始化DAC
    Drv_DAC_Init();
    
    // 初始化SI5351
    Drv_SI5351_Init();
    
    LOG_I("Radio Frequency module initialized");
}

/**************************End of file********************************/
