/***********************************************************************************
* @file     : app_negprsheat.c
* @brief    : Negative Pressure Heat treatment module implementation
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "app_negprsheat.h"
#include "app_treatmgr.h"
#include "app_memory.h"
#include "app_comm.h"
#include "drv_iodevice.h"
#include "drv_adc.h"
#include "log.h"
#include "drv_delay.h"
#include <string.h>

static NPH_CtrlInfo_t s_NPHCtrlInfo;

/**
 * @brief Convert pressure KPa to ADC voltage (需要根据实际硬件校准)
 * @param pressure_kpa Pressure in KPa (10-100)
 * @retval Target ADC voltage in mV (近似值，需要根据实际硬件校准)
 */
static uint16_t App_NegPrsHeat_PressureToVoltage(uint8_t pressure_kpa)
{
    // TODO: 根据实际硬件校准负压传感器
    // 这里使用线性近似：假设0KPa对应0V，100KPa对应3300mV
    // 实际需要根据硬件规格书校准
    if(pressure_kpa < NPH_PRESSURE_MIN_KPA) {
        pressure_kpa = NPH_PRESSURE_MIN_KPA;
    }
    if(pressure_kpa > NPH_PRESSURE_MAX_KPA) {
        pressure_kpa = NPH_PRESSURE_MAX_KPA;
    }
    // 负压值转换为电压值（需要根据实际传感器特性调整）
    // 假设线性关系：电压 = (pressure_kpa / 100) * 3300
    return (pressure_kpa * 3300) / 100;
}

/**
 * @brief Convert ADC voltage to pressure KPa (需要根据实际硬件校准)
 * @param voltage_mv ADC voltage in mV
 * @retval Pressure in KPa
 */
static uint8_t App_NegPrsHeat_VoltageToPressure(uint16_t voltage_mv)
{
    // TODO: 根据实际硬件校准负压传感器
    // 反向转换：pressure_kpa = (voltage_mv / 3300) * 100
    uint8_t pressure = (voltage_mv * 100) / 3300;
    if(pressure < NPH_PRESSURE_MIN_KPA) {
        pressure = NPH_PRESSURE_MIN_KPA;
    }
    if(pressure > NPH_PRESSURE_MAX_KPA) {
        pressure = NPH_PRESSURE_MAX_KPA;
    }
    return pressure;
}

void App_NegPrsHeat_UpdateStatus(void)
{
    Heat_TransData_t *pTransData = App_Comm_GetHeatTransData();
    
    // Update work state
    if(s_NPHCtrlInfo.runState == E_NPH_RUN_WORKING) {
        pTransData->TxStatus.work_state = 0x01;
    } else if(s_NPHCtrlInfo.runState == E_NPH_RUN_PREHEAT) {
        pTransData->TxStatus.preheat_state = 0x01;
        pTransData->TxStatus.work_state = 0x00;
    } else {
        pTransData->TxStatus.work_state = 0x00;
        pTransData->TxStatus.preheat_state = 0x00;
    }
    
    pTransData->TxStatus.temp_limit = s_NPHCtrlInfo.WorkTempLimit;
    pTransData->TxStatus.remain_heat_time = s_NPHCtrlInfo.RemainTime;
    pTransData->TxStatus.suck_time = s_NPHCtrlInfo.SuckTime;
    pTransData->TxStatus.release_time = s_NPHCtrlInfo.ReleaseTime;
    pTransData->TxStatus.pressure = s_NPHCtrlInfo.Pressure;
    pTransData->TxStatus.head_temp = s_NPHCtrlInfo.HeadTemp;
    pTransData->TxStatus.preheat_temp_limit = s_NPHCtrlInfo.PreheatTempLimit;
    pTransData->TxStatus.remain_preheat_time = s_NPHCtrlInfo.PreheatTime;
    
    // Get probe connection status and foot switch state
    bool headConnected = (s_NPHCtrlInfo.probeStatus == E_IODEVICE_MODE_NEGATIVE_PRESSURE_HEAT);
    
    // Combine connection state
    if (headConnected && s_NPHCtrlInfo.FootSwitchStatus) {
        pTransData->TxStatus.conn_state = CONN_STATE_CONNECTED_FOOT_CLOSED;
    } else if (!headConnected && s_NPHCtrlInfo.FootSwitchStatus) {
        pTransData->TxStatus.conn_state = CONN_STATE_DISCONNECTED_FOOT_CLOSED;
    } else if (headConnected && !s_NPHCtrlInfo.FootSwitchStatus) {
        pTransData->TxStatus.conn_state = CONN_STATE_CONNECTED_FOOT_OPEN;
    } else {
        pTransData->TxStatus.conn_state = CONN_STATE_DISCONNECTED_FOOT_OPEN;
    }
    pTransData->TxStatus.error_code = s_NPHCtrlInfo.ErrorCode;
}

void App_NegPrsHeat_RxDataHandle(void)
{
    Heat_TransData_t *pTransData = App_Comm_GetHeatTransData();
    
    if(pTransData->RxWorkState.work_state == WORK_STATE_RESET)
    {
        // 处理复位功能
        // 重置工作温度上限、工作时间、负压大小、负压吸时间、负压放时间
        s_NPHCtrlInfo.WorkTempLimit = pTransData->RxWorkState.temp_limit;
        s_NPHCtrlInfo.RemainTime = pTransData->RxWorkState.work_time;
        s_NPHCtrlInfo.Pressure = pTransData->RxWorkState.pressure;
        s_NPHCtrlInfo.SuckTime = pTransData->RxWorkState.suck_time;
        s_NPHCtrlInfo.ReleaseTime = pTransData->RxWorkState.release_time;
        
        // 剩余可治疗次数减一
        if(s_NPHCtrlInfo.TreatTimes > 0)
        {
            s_NPHCtrlInfo.TreatTimes--;
            // 保存到存储器
            s_NPHCtrlInfo.TreatParams.RemainTimes = s_NPHCtrlInfo.TreatTimes;
            App_Memory_SaveNPHParams(&s_NPHCtrlInfo.TreatParams);
            LOG_I("NPH Reset: Remaining treat times decreased to: %d", s_NPHCtrlInfo.TreatTimes);
        }
        
        LOG_I("NPH Reset: temp_limit=%d, work_time=%d, pressure=%d, suck=%d, release=%d", 
              s_NPHCtrlInfo.WorkTempLimit, s_NPHCtrlInfo.RemainTime, 
              s_NPHCtrlInfo.Pressure, s_NPHCtrlInfo.SuckTime, s_NPHCtrlInfo.ReleaseTime);
    }
    
    // 处理预热配置
    if(pTransData->flag.bits.Rely_Config)
    {
        // 预热配置通过RxPreheat结构体传递
        if(pTransData->RxPreheat.preheat_state == 0x01)
        {
            s_NPHCtrlInfo.PreheatEnable = true;
            s_NPHCtrlInfo.PreheatTempLimit = pTransData->RxPreheat.temp_limit;
            s_NPHCtrlInfo.PreheatTime = pTransData->RxPreheat.work_time;
        }
        else
        {
            s_NPHCtrlInfo.PreheatEnable = false;
        }
        
        // 保存到存储器
        s_NPHCtrlInfo.TreatParams.PreheatEnable = s_NPHCtrlInfo.PreheatEnable ? 1 : 0;
        s_NPHCtrlInfo.TreatParams.PreheatTempLimit = s_NPHCtrlInfo.PreheatTempLimit;
        s_NPHCtrlInfo.TreatParams.PreheatTime = s_NPHCtrlInfo.PreheatTime;
        App_Memory_SaveNPHParams(&s_NPHCtrlInfo.TreatParams);
        
        pTransData->flag.bits.Rely_Config = 0;
        LOG_I("NPH Config updated: preheat_enable=%d, preheat_temp=%d, preheat_time=%d", 
              s_NPHCtrlInfo.PreheatEnable, s_NPHCtrlInfo.PreheatTempLimit, s_NPHCtrlInfo.PreheatTime);
    }
}

void App_NegPrsHeat_ChangeState(NPH_RunState_EnumDef newState)
{
    if(newState != s_NPHCtrlInfo.runState && newState < E_NPH_RUN_MAX)
    {
        s_NPHCtrlInfo.runState = newState;
        switch(newState)
        {
            case E_NPH_RUN_INIT:
                LOG_I("NPH state changed to INIT");
                break;
            case E_NPH_RUN_IDLE:
                LOG_I("NPH state changed to IDLE");
                break;
            case E_NPH_RUN_PREHEAT:
                LOG_I("NPH state changed to PREHEAT");
                break;
            case E_NPH_RUN_WORKING:
                LOG_I("NPH state changed to WORKING");
                // 启动工作时蜂鸣器提示（2s）
                Drv_IODevice_StartBuzzer(2000);
                break;
            case E_NPH_RUN_STOP:
                LOG_I("NPH state changed to STOP");
                // 结束工作时蜂鸣器提示（2s）
                Drv_IODevice_StartBuzzer(2000);
                break;
            default:
                break;
        }
    }
}

void App_NegPrsHeat_Monitor(void)
{
    // Get the foot switch status and probe status
    s_NPHCtrlInfo.FootSwitchStatus = Drv_IODevice_GetFootSwitchState();
    
    // Get the probe status
    s_NPHCtrlInfo.probeStatus = Drv_IODevice_GetProbeStatus();
}

bool App_NegPrsHeat_StartCheck()
{
    Heat_TransData_t *pTransData = App_Comm_GetHeatTransData();
    
    // 1. 检查下位机是否下发了发射负压加热指令
    if(pTransData->RxWorkState.work_state != WORK_STATE_START) {
        LOG_E("NPH: Work state is not start");
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 2. 检查剩余工作时间是否大于0（0-3600s）
    if(pTransData->RxWorkState.work_time == 0 || pTransData->RxWorkState.work_time > NPH_WORK_TIME_MAX) {
        LOG_E("NPH: Invalid work time: %d", pTransData->RxWorkState.work_time);
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 3. 检查负压大小是否有效（10-100KPa）
    if(pTransData->RxWorkState.pressure < NPH_PRESSURE_MIN_KPA || 
       pTransData->RxWorkState.pressure > NPH_PRESSURE_MAX_KPA) {
        LOG_E("NPH: Invalid pressure: %d (range: %d-%d)", 
              pTransData->RxWorkState.pressure, NPH_PRESSURE_MIN_KPA, NPH_PRESSURE_MAX_KPA);
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 4. 检查负压吸时间是否有效（0.1-60s，单位100ms）
    if(pTransData->RxWorkState.suck_time < (NPH_SUCK_TIME_MIN_MS/100) || 
       pTransData->RxWorkState.suck_time > (NPH_SUCK_TIME_MAX_MS/100)) {
        LOG_E("NPH: Invalid suck time: %d (range: %d-%d)", 
              pTransData->RxWorkState.suck_time, NPH_SUCK_TIME_MIN_MS/100, NPH_SUCK_TIME_MAX_MS/100);
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 5. 检查负压放时间是否有效（0.1-60s，单位100ms）
    if(pTransData->RxWorkState.release_time < (NPH_RELEASE_TIME_MIN_MS/100) || 
       pTransData->RxWorkState.release_time > (NPH_RELEASE_TIME_MAX_MS/100)) {
        LOG_E("NPH: Invalid release time: %d (range: %d-%d)", 
              pTransData->RxWorkState.release_time, NPH_RELEASE_TIME_MIN_MS/100, NPH_RELEASE_TIME_MAX_MS/100);
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 6. 检查脚踏开关是否闭合
    if(s_NPHCtrlInfo.FootSwitchStatus == false) {
        LOG_E("NPH: Foot switch is not closed");
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 7. 检查是否正确识别到负压加热治疗头
    if(s_NPHCtrlInfo.probeStatus != E_IODEVICE_MODE_NEGATIVE_PRESSURE_HEAT) {
        LOG_E("NPH: Negative pressure heat probe not connected");
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_PROBE_NOT_CONNECTED;
        return false;
    }
    
    // 8. 检查是否有剩余可治疗次数
    if(s_NPHCtrlInfo.TreatTimes == 0) {
        LOG_E("NPH: No remaining treat times");
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 所有检查通过
    return true;
}

void App_NegPrsHeat_SetWorkParams(void)
{
    Heat_TransData_t *pTransData = App_Comm_GetHeatTransData();
    
    // 设置工作参数
    s_NPHCtrlInfo.WorkTempLimit = pTransData->RxWorkState.temp_limit;
    s_NPHCtrlInfo.RemainTime = pTransData->RxWorkState.work_time;
    s_NPHCtrlInfo.Pressure = pTransData->RxWorkState.pressure;
    s_NPHCtrlInfo.SuckTime = pTransData->RxWorkState.suck_time;  // 单位：100ms
    s_NPHCtrlInfo.ReleaseTime = pTransData->RxWorkState.release_time;  // 单位：100ms
    
    // 计算目标负压值（转换为电压）
    s_NPHCtrlInfo.targetPressure = App_NegPrsHeat_PressureToVoltage(s_NPHCtrlInfo.Pressure);
    
    // 切换继电器pwr_control1至负压加热通道
    Drv_IODevice_ChangeChannel(CHANNEL_READY);
    
    // 初始化负压状态
    s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_IDLE;
    s_NPHCtrlInfo.motorState = false;
    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 0);
    
    // 初始化加热控制
    s_NPHCtrlInfo.heatControlActive = false;
    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 0);
    
    LOG_I("NPH: Work params set - temp_limit=%d, work_time=%d, pressure=%d, suck=%d, release=%d", 
          s_NPHCtrlInfo.WorkTempLimit, s_NPHCtrlInfo.RemainTime, 
          s_NPHCtrlInfo.Pressure, s_NPHCtrlInfo.SuckTime, s_NPHCtrlInfo.ReleaseTime);
}

bool App_NegPrsHeat_IsHeadTempNormal(void)
{
    uint16_t temp = Drv_ADC_GetRealValue(E_ADC_CHANNEL_HAND_NTC);
    uint32_t currentTime = Drv_Delay_GetTickMs();
    bool isNormal = true;
    
    // 检查温度传感器是否出错（NTC开路或短路）
    if(temp == 0xFFFF || temp == 0xEEFF)
    {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_TEMP_SENSOR_ERROR;
        LOG_W("NPH: Temperature sensor error: %d", temp);
        isNormal = false;
    }
    // 检查温度是否超过65℃（650 * 0.1°C）
    else if(temp > NPH_TEMP_ERROR_THRESHOLD)
    {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_TEMP_TOO_HIGH;
        LOG_W("NPH: Head temperature too high: %d (threshold: %d)", temp, NPH_TEMP_ERROR_THRESHOLD);
        isNormal = false;
    }
    // 检查温度是否在2s内上升至65℃
    if(s_NPHCtrlInfo.lastTemp > 0)
    {
        uint16_t tempRise = temp - s_NPHCtrlInfo.lastTemp;
        uint32_t timeElapsed = currentTime - s_NPHCtrlInfo.tempErrorStartTime;
        
        // 如果温度上升且时间在2s内
        if(tempRise > 0 && timeElapsed <= NPH_TEMP_ERROR_TIME_MS)
        {
            // 如果温度达到或超过65℃，且是在2s内上升的，则报警
            if(temp >= NPH_TEMP_ERROR_THRESHOLD)
            {
                s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_TEMP_RISE_TOO_FAST;
                LOG_W("NPH: Temperature rise to 65 degree in %d ms (from %d to %d)", 
                      timeElapsed, s_NPHCtrlInfo.lastTemp, temp);
                isNormal = false;
            }
        }
        
        // 如果温度下降或时间超过2s，重置错误检测起始时间
        if(tempRise <= 0 || timeElapsed > NPH_TEMP_ERROR_TIME_MS)
        {
            s_NPHCtrlInfo.tempErrorStartTime = currentTime;
            s_NPHCtrlInfo.lastTemp = temp;  // 更新基准温度
        }
    }
    else
    {
        // 首次读取，初始化错误检测起始时间和基准温度
        s_NPHCtrlInfo.tempErrorStartTime = currentTime;
        s_NPHCtrlInfo.lastTemp = temp;
    }
    
    s_NPHCtrlInfo.HeadTemp = temp;
    
    if(isNormal && s_NPHCtrlInfo.ErrorCode != E_NPH_ERROR_NONE)
    {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_NONE;
    }
    
    return isNormal;
}

void App_NegPrsHeat_ControlTemperature(void)
{
    uint16_t temp = s_NPHCtrlInfo.HeadTemp;
    uint16_t targetTemp = s_NPHCtrlInfo.WorkTempLimit;
    bool needHeat = false;
    
    // 根据当前状态确定目标温度
    if(s_NPHCtrlInfo.runState == E_NPH_RUN_PREHEAT)
    {
        targetTemp = s_NPHCtrlInfo.PreheatTempLimit;
    }
    else if(s_NPHCtrlInfo.runState == E_NPH_RUN_WORKING)
    {
        targetTemp = s_NPHCtrlInfo.WorkTempLimit;
    }
    else
    {
        // 不在工作状态，关闭加热
        Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 0);
        s_NPHCtrlInfo.heatControlActive = false;
        return;
    }
    
    // 简单的开关控制：温度低于目标温度时加热，高于目标温度时停止加热
    // 可以添加死区（hysteresis）来避免频繁开关
    if(temp < targetTemp - 5)  // 低于目标温度5*0.1℃时开始加热
    {
        needHeat = true;
    }
    else if(temp > targetTemp + 5)  // 高于目标温度5*0.1℃时停止加热
    {
        needHeat = false;
    }
    else
    {
        // 在死区内，保持当前状态
        needHeat = s_NPHCtrlInfo.heatControlActive;
    }
    
    // 控制CTR_HEAT_HP
    if(needHeat != s_NPHCtrlInfo.heatControlActive)
    {
        Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, needHeat ? 1 : 0);
        s_NPHCtrlInfo.heatControlActive = needHeat;
        LOG_I("NPH: Heat control %s (temp=%d, target=%d)", 
              needHeat ? "ON" : "OFF", temp, targetTemp);
    }
}

void App_NegPrsHeat_ProcessVacuum(void)
{
	uint16_t targetVoltage;
	uint32_t maintainElapsed;
	uint32_t maintainTimeMs;
	uint32_t releaseElapsed;  
	uint32_t releaseTimeMs;	
//	int16_t voltageDiff;
    uint32_t currentTime = Drv_Delay_GetTickMs();
    uint16_t pressureVoltage = Drv_ADC_GetRealValue(E_ADC_CHANNEL_HP_PRE);
    s_NPHCtrlInfo.currentPressure = App_NegPrsHeat_VoltageToPressure(pressureVoltage);
    
    switch(s_NPHCtrlInfo.vacuumState)
    {
        case E_NPH_VACUUM_STATE_IDLE:
            // 开始吸气
            s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_SUCKING;
            s_NPHCtrlInfo.vacuumStateStartTime = currentTime;
            s_NPHCtrlInfo.suckStartTime = currentTime;
            s_NPHCtrlInfo.motorState = true;
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 1);
            LOG_I("NPH: Start sucking, target pressure: %d KPa", s_NPHCtrlInfo.Pressure);
            break;
            
        case E_NPH_VACUUM_STATE_SUCKING:
            // 检查是否达到目标负压（负压值越大，压力越小，所以currentPressure应该大于等于targetPressure）
            // 注意：这里需要根据实际硬件特性调整判断逻辑
            // 如果ADC采样值越大表示负压越大，则应该判断currentPressure >= targetPressure
            // 如果ADC采样值越大表示压力越大（负压越小），则需要反向判断
            targetVoltage = App_NegPrsHeat_PressureToVoltage(s_NPHCtrlInfo.Pressure);
            if(pressureVoltage >= targetVoltage)
            {
                // 达到目标负压，进入维持状态
                s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_MAINTAIN;
                s_NPHCtrlInfo.maintainStartTime = currentTime;
                LOG_I("NPH: Target pressure reached (%d KPa), start maintaining", s_NPHCtrlInfo.Pressure);
            }
            else
            {
                // 继续吸气，保持电机运行
                if(!s_NPHCtrlInfo.motorState)
                {
                    s_NPHCtrlInfo.motorState = true;
                    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 1);
                }
            }
            break;
            
        case E_NPH_VACUUM_STATE_MAINTAIN:
            // 维持负压大小
            // 检查维持时间是否到达
            maintainElapsed = currentTime - s_NPHCtrlInfo.maintainStartTime;
            maintainTimeMs = s_NPHCtrlInfo.SuckTime * 100;  // 转换为毫秒
            
            if(maintainElapsed >= maintainTimeMs)
            {
                // 维持时间到，开始放气
                s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_RELEASING;
                s_NPHCtrlInfo.releaseStartTime = currentTime;
                s_NPHCtrlInfo.motorState = false;
                Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 0);
                // 打开释放阀（如果有的话，这里使用CTR_HP_lose）
                Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_LOSE, 1);
                LOG_I("NPH: Maintain time reached, start releasing");
            }
            else
            {
				
                // 在维持时间内，通过控制电机维持负压
                targetVoltage = App_NegPrsHeat_PressureToVoltage(s_NPHCtrlInfo.Pressure);
//                voltageDiff = (pressureVoltage > targetVoltage) ? 
//                                       (pressureVoltage - targetVoltage) : 
//                                       (targetVoltage - pressureVoltage);
                uint16_t thresholdVoltage = App_NegPrsHeat_PressureToVoltage(5);  // 5KPa对应的电压差
                
                if(pressureVoltage < targetVoltage - thresholdVoltage)  // 负压不足（电压低于目标）
                {
                    // 负压不足，启动电机补充
                    if(!s_NPHCtrlInfo.motorState)
                    {
                        s_NPHCtrlInfo.motorState = true;
                        Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 1);
                    }
                }
                else if(pressureVoltage > targetVoltage + thresholdVoltage)  // 负压过高（电压高于目标）
                {
                    // 负压过高，停止电机
                    if(s_NPHCtrlInfo.motorState)
                    {
                        s_NPHCtrlInfo.motorState = false;
                        Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 0);
                    }
                }
            }
            break;
        	
        case E_NPH_VACUUM_STATE_RELEASING:
            // 放气状态
            releaseElapsed = currentTime - s_NPHCtrlInfo.releaseStartTime;
            releaseTimeMs = s_NPHCtrlInfo.ReleaseTime * 100;  // 转换为毫秒
            
            if(releaseElapsed >= releaseTimeMs)
            {
                // 放气时间到，关闭释放阀，准备下一个循环
                Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_LOSE, 0);
                s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_IDLE;
                LOG_I("NPH: Release time reached, ready for next cycle");
            }
            break;
            
        default:
            break;
    }
}

void App_NegPrsHeat_Process(void)
{
    static Drv_Timer_t TempMonitorTimer;
    
    // Process the negative pressure heat module
    App_NegPrsHeat_UpdateStatus();
    App_NegPrsHeat_RxDataHandle();
    App_NegPrsHeat_Monitor();

    // Handle the negative pressure heat state
    switch(s_NPHCtrlInfo.runState)
    {
        case E_NPH_RUN_INIT:
            // 加载负压加热参数
            if(App_Memory_LoadNPHParams(&s_NPHCtrlInfo.TreatParams)) {
                s_NPHCtrlInfo.TempLimit = s_NPHCtrlInfo.TreatParams.TempLimit;
                s_NPHCtrlInfo.TreatTimes = s_NPHCtrlInfo.TreatParams.RemainTimes;
                s_NPHCtrlInfo.PreheatEnable = (s_NPHCtrlInfo.TreatParams.PreheatEnable == 1);
                s_NPHCtrlInfo.PreheatTempLimit = s_NPHCtrlInfo.TreatParams.PreheatTempLimit;
                s_NPHCtrlInfo.PreheatTime = s_NPHCtrlInfo.TreatParams.PreheatTime;
                
                LOG_I("NPH: Parameters loaded - temp_limit=%d, remain_times=%d, preheat_enable=%d, preheat_temp=%d, preheat_time=%d",
                      s_NPHCtrlInfo.TempLimit, s_NPHCtrlInfo.TreatTimes,
                      s_NPHCtrlInfo.PreheatEnable, s_NPHCtrlInfo.PreheatTempLimit, s_NPHCtrlInfo.PreheatTime);
            } else {
                LOG_E("NPH: Failed to load parameters");
                s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_READ_PARAMS_FAILED;
            }
            App_NegPrsHeat_ChangeState(E_NPH_RUN_IDLE);
            break;
            
        case E_NPH_RUN_IDLE:
            // 使用App_NegPrsHeat_StartCheck进行启动前检查
            if(App_NegPrsHeat_StartCheck()) {
                // 设置工作参数
                App_NegPrsHeat_SetWorkParams();
                
                // 如果开启了预热功能，先进入预热状态
                if(s_NPHCtrlInfo.PreheatEnable)
                {
                    App_NegPrsHeat_ChangeState(E_NPH_RUN_PREHEAT);
                }
                else
                {
                    // 直接进入工作状态
                    Drv_IODevice_ChangeChannel(CHANNEL_NH);
                    App_NegPrsHeat_ChangeState(E_NPH_RUN_WORKING);
                }
            }
            break;
            
        case E_NPH_RUN_PREHEAT:
            // 预热状态：加热至预热温度上限
            // 检查温度是否达到预热温度上限
            if(s_NPHCtrlInfo.HeadTemp >= s_NPHCtrlInfo.PreheatTempLimit)
            {
                // 预热完成，进入工作状态
                Drv_IODevice_ChangeChannel(CHANNEL_NH);
                App_NegPrsHeat_ChangeState(E_NPH_RUN_WORKING);
                LOG_I("NPH: Preheat completed, entering working state");
            }
            // 检查所有条件
            else if(App_NegPrsHeat_StartCheck() == false)
            {
                App_NegPrsHeat_ChangeState(E_NPH_RUN_STOP);
            }
            else
            {
                // 温度监控（10ms周期）
                if(Drv_Timer_Tick(&TempMonitorTimer, NPH_TEMP_MONITOR_PERIOD_MS)) {
                    if(App_NegPrsHeat_IsHeadTempNormal() == false) {
                        // 温度异常，立马停止
                        App_NegPrsHeat_ChangeState(E_NPH_RUN_STOP);
                    } else {
                        // 控制温度
                        App_NegPrsHeat_ControlTemperature();
                    }
                }
            }
            break;
            
        case E_NPH_RUN_WORKING:
            // 检查所有条件
            if(App_NegPrsHeat_StartCheck() == false || 
               s_NPHCtrlInfo.RemainTime == 0){
                App_NegPrsHeat_ChangeState(E_NPH_RUN_STOP);
            } else {
                // 温度监控（10ms周期）
                if(Drv_Timer_Tick(&TempMonitorTimer, NPH_TEMP_MONITOR_PERIOD_MS)) {
                    if(App_NegPrsHeat_IsHeadTempNormal() == false) {
                        // 温度异常，立马停止
                        App_NegPrsHeat_ChangeState(E_NPH_RUN_STOP);
                    } else {
                        // 控制温度
                        App_NegPrsHeat_ControlTemperature();
                    }
                }
                
                // 处理负压控制
                App_NegPrsHeat_ProcessVacuum();
                
                // 更新时间
                if(s_NPHCtrlInfo.RemainTime >= TREAT_TASK_TIME) {
                    s_NPHCtrlInfo.RemainTime -= TREAT_TASK_TIME;
                } else {
                    s_NPHCtrlInfo.RemainTime = 0;
                }
            }
            break;
            
        case E_NPH_RUN_STOP:
            // 关闭加热
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 0);
            s_NPHCtrlInfo.heatControlActive = false;
            
            // 关闭负压控制
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 0);
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_LOSE, 0);
            s_NPHCtrlInfo.motorState = false;
            s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_IDLE;
            
            // 关闭输出通道
            Drv_IODevice_ChangeChannel(CHANNEL_CLOSE);
            App_TreatMgr_ChangeState(E_TREATMGR_STATE_IDLE);
            break;
            
        default:
            break;
    }
}

/**
 * @brief Initialize negative pressure heat module
 */
void App_NegPrsHeat_Init(void)
{
    // 初始化控制信息结构
    memset(&s_NPHCtrlInfo, 0, sizeof(NPH_CtrlInfo_t));
    
    // 设置初始状态
    s_NPHCtrlInfo.runState = E_NPH_RUN_INIT;
    s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_IDLE;
    s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_NONE;
    s_NPHCtrlInfo.RemainTime = 0;
    s_NPHCtrlInfo.TreatTimes = 0;
    s_NPHCtrlInfo.heatControlActive = false;
    s_NPHCtrlInfo.motorState = false;
    
    // 确保所有控制引脚为低电平
    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 0);
    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 0);
    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_LOSE, 0);
    
    LOG_I("Negative Pressure Heat module initialized");
}

/**************************End of file********************************/
