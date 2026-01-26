/***********************************************************************************
* @file     : app_treatmgr.c
* @brief    : 
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "app_treatmgr.h"
#include "drv_iodevice.h"
#include "drv_adc.h"
#include "log.h"
#include "app_ultrasound.h"
#include "app_shockwave.h"
#include "app_radiofreq.h"
#include "app_negprsheat.h"
#include "drv_delay.h"

TreatMgr_t s_TreatMgr;

/* 板上温度监控参数 */
#define BOARD_TEMP_FAN_ON_THRESHOLD     850     ///< 风扇启动温度阈值 (85℃ = 850 * 0.1°C)
#define BOARD_TEMP_FAN_OFF_THRESHOLD    800     ///< 风扇关闭温度阈值 (80℃ = 800 * 0.1°C)，添加回差避免频繁开关
#define BOARD_TEMP_MONITOR_PERIOD_MS    1000    ///< 板上温度监控周期 (1s)

/**
 * @brief Read board temperature from Heat_REF01 and Heat_REF02
 * @retval Board temperature in 0.1°C, returns average of two sensors
 * @note 需要根据实际温度传感器特性进行校准
 *       如果是NTC热敏电阻，需要使用Steinhart-Hart方程或查表法
 *       如果是线性传感器，使用线性转换公式
 */
static uint16_t App_TreatMgr_ReadBoardTemp(void)
{
    uint16_t temp1_voltage = Drv_ADC_ReadVoltage(E_ADC_CHANNEL_Heat_REF01);
    uint16_t temp2_voltage = Drv_ADC_ReadVoltage(E_ADC_CHANNEL_Heat_REF02);
    
    // TODO: 根据实际温度传感器特性转换电压到温度
    // 这里使用简化的线性转换作为占位实现
    // 实际需要根据传感器规格书进行校准：
    // - 如果是NTC热敏电阻：需要使用Steinhart-Hart方程或查表法
    // - 如果是线性传感器：使用线性转换公式
    // - 如果是其他类型传感器：根据其特性曲线转换
    
    // 取两个传感器的平均值（如果两个传感器都有效）
    uint16_t avg_voltage = (temp1_voltage + temp2_voltage) / 2;
    
    // 简化的线性转换（需要根据实际硬件校准）
    // 假设：0V = 0℃，3.3V = 100℃ = 1000 * 0.1°C
    // 温度 = (voltage_mv * 1000) / 3300
    // 注意：这个转换公式是占位实现，需要根据实际传感器特性替换
    uint16_t board_temp = (avg_voltage * 1000) / 3300;
    
    return board_temp;
}

/**
 * @brief Control fan based on board temperature
 */
static void App_TreatMgr_ControlFan(void)
{
    static bool fanState = false;
    uint16_t boardTemp = App_TreatMgr_ReadBoardTemp();
    
    if(boardTemp > BOARD_TEMP_FAN_ON_THRESHOLD)
    {
        // 温度超过85℃，启动风扇
        if(!fanState)
        {
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_FAN, 1);
            fanState = true;
            LOG_I("Board temperature too high (%d * 0.1°C), fan started", boardTemp);
        }
    }
    else if(boardTemp < BOARD_TEMP_FAN_OFF_THRESHOLD)
    {
        // 温度低于80℃，关闭风扇（添加回差避免频繁开关）
        if(fanState)
        {
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_FAN, 0);
            fanState = false;
            LOG_I("Board temperature normal (%d * 0.1°C), fan stopped", boardTemp);
        }
    }
    // 在80-85℃之间保持当前状态
}

void App_TreatMgr_Init(void)
{
    // Initialize the treatment manager module
    s_TreatMgr.eState = E_TREATMGR_STATE_IDLE;
}

void App_TreatMgr_ChangeState(TreatMgr_State_EnumDef newState)
{
    if(newState != s_TreatMgr.eState && newState < E_TREATMGR_STATE_MAX)
    {
        s_TreatMgr.eState = newState;
        s_TreatMgr.preState = s_TreatMgr.eState;
        switch(newState)
        {
            case E_TREATMGR_STATE_IDLE:
                LOG_I("TreatMgr state changed to IDLE");
                break;
            case E_TREATMGR_STATE_RADIO_FREQUENCY:
                LOG_I("TreatMgr state changed to RADIO_FREQUENCY");
                break;
            case E_TREATMGR_STATE_SHOCK_WAVE:
                LOG_I("TreatMgr state changed to SHOCK_WAVE");
                break;
            case E_TREATMGR_STATE_NEGATIVE_PRESSURE_HEAT:
                LOG_I("TreatMgr state changed to NEGATIVE_PRESSURE_HEAT");
                break;
            case E_TREATMGR_STATE_ULTRASOUND:
                LOG_I("TreatMgr state changed to ULTRASOUND");
                break;
            case E_TREATMGR_STATE_ERROR:
                LOG_I("TreatMgr state changed to ERROR");
                break;
        }
    }
}


void ProbeStatusCheck()
{
    s_TreatMgr.eProbeStatus = Drv_IODevice_GetProbeStatus();
    if(s_TreatMgr.eProbeStatus != s_TreatMgr.preProbeStaus){
        s_TreatMgr.preProbeStaus = s_TreatMgr.eProbeStatus;
        switch(s_TreatMgr.eProbeStatus)
        {
            case E_IODEVICE_MODE_ULTRASOUND:
                LOG_I("Probe status changed to ULTRASOUND");
                break;
            case E_IODEVICE_MODE_SHOCKWAVE:
                LOG_I("Probe status changed to SHOCKWAVE");
                break;
            case E_IODEVICE_MODE_RADIO_FREQUENCY:
                LOG_I("Probe status changed to RADIO_FREQUENCY");
                break;
            case E_IODEVICE_MODE_NEGATIVE_PRESSURE_HEAT:
                LOG_I("Probe status changed to NEGATIVE_PRESSURE_HEAT");
                break;
            case E_IODEVICE_MODE_NOT_CONNECTED:
                LOG_I("Probe status changed to NOT_CONNECTED");
                break;  
            case E_IODEVICE_MODE_ERROR:
                LOG_I("Probe status changed to ERROR");
                break;
            default:
                LOG_I("Probe status changed to UNKNOWN");
                break;
        }
    }
}

// void App_TreatMgr_ChangeCheck(IODevice_WorkingMode_EnumDef curProbe) 
// {
//     if(curProbe != s_TreatMgr.preProbeStaus){
        
//     }
// }

void App_TreatMgr_Process(void)
{
    static Drv_Timer_t TreatMgrTimer;
    static Drv_Timer_t BoardTempMonitorTimer;

    // 处理蜂鸣器控制（每次循环都处理，确保及时响应）
    Drv_IODevice_ProcessBuzzer();
    
    if(Drv_Timer_Tick(&TreatMgrTimer, TREAT_TASK_TIME) == false){
        return;
    }
    // Process the treatment manager module
    ProbeStatusCheck();
    
    // 板上温度监控和风扇控制（1s周期）
    if(Drv_Timer_Tick(&BoardTempMonitorTimer, BOARD_TEMP_MONITOR_PERIOD_MS)){
        App_TreatMgr_ControlFan();
    }
    switch(s_TreatMgr.eState)
    {
        case E_TREATMGR_STATE_IDLE:
            // Handle idle state
            switch(s_TreatMgr.eProbeStatus)
            {
                case E_IODEVICE_MODE_ULTRASOUND:
                    App_TreatMgr_ChangeState(E_TREATMGR_STATE_ULTRASOUND);
                    break;
                case E_IODEVICE_MODE_SHOCKWAVE:
                    App_TreatMgr_ChangeState(E_TREATMGR_STATE_SHOCK_WAVE);
                    break;
                case E_IODEVICE_MODE_RADIO_FREQUENCY:
                    App_TreatMgr_ChangeState(E_TREATMGR_STATE_RADIO_FREQUENCY);
                    break;
                case E_IODEVICE_MODE_NEGATIVE_PRESSURE_HEAT:
                    App_TreatMgr_ChangeState(E_TREATMGR_STATE_NEGATIVE_PRESSURE_HEAT);
                    break;
                case E_IODEVICE_MODE_NOT_CONNECTED:
                    break;
                case E_IODEVICE_MODE_ERROR:
                    App_TreatMgr_ChangeState(E_TREATMGR_STATE_ERROR);
                    break;
            }
            break;
        case E_TREATMGR_STATE_RADIO_FREQUENCY:
            // Handle radio frequency state
            App_RadioFreq_Process();
            break;
        case E_TREATMGR_STATE_SHOCK_WAVE:
            // Handle shock wave state
            App_Shockwave_Process();
            break;
        case E_TREATMGR_STATE_NEGATIVE_PRESSURE_HEAT:
            // Handle negative pressure heat state
            App_NegPrsHeat_Process();
            break;
        case E_TREATMGR_STATE_ULTRASOUND:
            // Handle ultrasound state
            App_Ultrasound_Process();
            break;
        case E_TREATMGR_STATE_ERROR:
            // Handle error state
            break;
		case E_TREATMGR_STATE_MAX:
			break;
    }
}

/**************************End of file********************************/
