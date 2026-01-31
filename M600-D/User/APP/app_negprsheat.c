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
 * @brief Convert pressure KPa to ADC voltage 
 * @param pressure_kpa Pressure in KPa (10-100)
 * @retval Target ADC voltage in mV
 */
static uint16_t App_NegPrsHeat_PressureToVoltage(uint8_t pressure_kpa)
{

    if(pressure_kpa < NPH_PRESSURE_MIN_KPA) {
        pressure_kpa = NPH_PRESSURE_MIN_KPA;
    }
    if(pressure_kpa > NPH_PRESSURE_MAX_KPA) {
        pressure_kpa = NPH_PRESSURE_MAX_KPA;
    }

    return (pressure_kpa * 3300) / 100;
}

/**
 * @brief Convert ADC voltage to pressure KPa
 * @param voltage_mv ADC voltage in mV
 * @retval Pressure in KPa
 */
static uint8_t App_NegPrsHeat_VoltageToPressure(uint16_t voltage_mv)
{
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
    pTransData->TxStatus.remain_heat_time = s_NPHCtrlInfo.TreatRemainTimes / 100;  /* 10ms -> s */
    pTransData->TxStatus.suck_time = s_NPHCtrlInfo.SuckTime;
    pTransData->TxStatus.release_time = s_NPHCtrlInfo.ReleaseTime;
    pTransData->TxStatus.pressure = s_NPHCtrlInfo.Pressure;
    pTransData->TxStatus.head_temp = s_NPHCtrlInfo.HeadTemp;
    pTransData->TxStatus.preheat_temp_limit = s_NPHCtrlInfo.PreheatTempLimit;
    pTransData->TxStatus.remain_preheat_time = s_NPHCtrlInfo.PreheatTime;
    
    // ???????? mgr ??
    bool headConnected = (App_TreatMgr_GetProbeStatus() == E_IODEVICE_MODE_NEGATIVE_PRESSURE_HEAT);
    bool footClosed = App_TreatMgr_GetFootSwitchClosed();
    if (headConnected && footClosed) {
        pTransData->TxStatus.conn_state = CONN_STATE_CONNECTED_FOOT_CLOSED;
    } else if (!headConnected && footClosed) {
        pTransData->TxStatus.conn_state = CONN_STATE_DISCONNECTED_FOOT_CLOSED;
    } else if (headConnected && !footClosed) {
        pTransData->TxStatus.conn_state = CONN_STATE_CONNECTED_FOOT_OPEN;
    } else {
        pTransData->TxStatus.conn_state = CONN_STATE_DISCONNECTED_FOOT_OPEN;
    }
    pTransData->TxStatus.error_code = s_NPHCtrlInfo.ErrorCode;
}

void App_NegPrsHeat_RxDataHandle(void)
{
    Heat_TransData_t *pTransData = App_Comm_GetHeatTransData();
    
    /* RxWorkState ? WorkTimeHandle ???? */
    
    // ??????
    if(pTransData->flag.bits.Rely_Config)
    {
        // ??????RxPreheat??????
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
        
        // ??????
        s_NPHCtrlInfo.TreatParams.PreheatEnable = s_NPHCtrlInfo.PreheatEnable ? 1 : 0;
        s_NPHCtrlInfo.TreatParams.PreheatTempLimit = s_NPHCtrlInfo.PreheatTempLimit;
        s_NPHCtrlInfo.TreatParams.PreheatTime = s_NPHCtrlInfo.PreheatTime;
        App_Memory_SaveNPHParams(&s_NPHCtrlInfo.TreatParams);
        
        pTransData->flag.bits.Rely_Config = 0;
        LOG_I("NPH Config updated: preheat_enable=%d, preheat_temp=%d, preheat_time=%d", 
              s_NPHCtrlInfo.PreheatEnable, s_NPHCtrlInfo.PreheatTempLimit, s_NPHCtrlInfo.PreheatTime);
    }
}

void App_NegPrsHeat_WorkTimeHandle(void)
{
    Heat_TransData_t *pTransData = App_Comm_GetHeatTransData();
    static uint8_t s_lastRxWorkState = 0x00;

    if(pTransData->RxWorkState.work_state != s_lastRxWorkState) {
        if (pTransData->RxWorkState.work_state == WORK_STATE_RESET && s_lastRxWorkState != WORK_STATE_RESET) {
            s_NPHCtrlInfo.TreatCountsState = E_TREAT_TIMES_RESET;
        }
        s_lastRxWorkState = pTransData->RxWorkState.work_state;
    }

    switch(s_NPHCtrlInfo.TreatCountsState)
    {
        case E_TREAT_TIMES_POWER_ON:
            if(pTransData->RxWorkState.work_time > 0 && s_NPHCtrlInfo.TreatCounts > 0)
            {
                s_NPHCtrlInfo.TreatRemainTimes = pTransData->RxWorkState.work_time * 100;  /* s -> 10ms */
                s_NPHCtrlInfo.WorkTempLimit = pTransData->RxWorkState.temp_limit;
                s_NPHCtrlInfo.Pressure = pTransData->RxWorkState.pressure;
                s_NPHCtrlInfo.SuckTime = pTransData->RxWorkState.suck_time;
                s_NPHCtrlInfo.ReleaseTime = pTransData->RxWorkState.release_time;
                s_NPHCtrlInfo.TreatCountsState = E_TREAT_TIMES_WORKING;
                s_NPHCtrlInfo.TreatCounts--;
                s_NPHCtrlInfo.TreatParams.TreatRemainTimes = s_NPHCtrlInfo.TreatCounts;
                App_Memory_SaveNPHParams(&s_NPHCtrlInfo.TreatParams);
                LOG_I("NPH: Remaining treat times decreased to: %d", s_NPHCtrlInfo.TreatCounts);
            }
            break;
        case E_TREAT_TIMES_RESET:
            if(pTransData->RxWorkState.work_time > 0 && s_NPHCtrlInfo.TreatCounts > 0)
            {
                s_NPHCtrlInfo.TreatRemainTimes = pTransData->RxWorkState.work_time * 100;  /* s -> 10ms */
                s_NPHCtrlInfo.WorkTempLimit = pTransData->RxWorkState.temp_limit;
                s_NPHCtrlInfo.Pressure = pTransData->RxWorkState.pressure;
                s_NPHCtrlInfo.SuckTime = pTransData->RxWorkState.suck_time;
                s_NPHCtrlInfo.ReleaseTime = pTransData->RxWorkState.release_time;
                s_NPHCtrlInfo.TreatCountsState = E_TREAT_TIMES_WORKING;
                s_NPHCtrlInfo.TreatCounts--;
                s_NPHCtrlInfo.TreatParams.TreatRemainTimes = s_NPHCtrlInfo.TreatCounts;
                App_Memory_SaveNPHParams(&s_NPHCtrlInfo.TreatParams);
                LOG_I("NPH: Remaining treat times decreased to: %d", s_NPHCtrlInfo.TreatCounts);
            }
            break;
        case E_TREAT_TIMES_WORKING:
            if(s_NPHCtrlInfo.TreatRemainTimes > 0 && s_NPHCtrlInfo.runState == E_NPH_RUN_WORKING)
            {
                if(s_NPHCtrlInfo.TreatRemainTimes >= TREAT_TASK_TIME) {
                    s_NPHCtrlInfo.TreatRemainTimes -= TREAT_TASK_TIME;
                } else {
                    s_NPHCtrlInfo.TreatRemainTimes = 0;
                }
            }
            if(s_NPHCtrlInfo.TreatRemainTimes == 0)
            {
                s_NPHCtrlInfo.TreatCountsState = E_TREAT_TIMES_WAIT;
            }
            break;
        case E_TREAT_TIMES_WAIT:
            if(pTransData->RxWorkState.work_time > 0 && s_NPHCtrlInfo.TreatCounts > 0)
            {
                s_NPHCtrlInfo.TreatRemainTimes = pTransData->RxWorkState.work_time * 100;  /* s -> 10ms */
                s_NPHCtrlInfo.WorkTempLimit = pTransData->RxWorkState.temp_limit;
                s_NPHCtrlInfo.Pressure = pTransData->RxWorkState.pressure;
                s_NPHCtrlInfo.SuckTime = pTransData->RxWorkState.suck_time;
                s_NPHCtrlInfo.ReleaseTime = pTransData->RxWorkState.release_time;
                s_NPHCtrlInfo.TreatCountsState = E_TREAT_TIMES_WORKING;
                s_NPHCtrlInfo.TreatCounts--;
                s_NPHCtrlInfo.TreatParams.TreatRemainTimes = s_NPHCtrlInfo.TreatCounts;
                App_Memory_SaveNPHParams(&s_NPHCtrlInfo.TreatParams);
                LOG_I("NPH: Remaining treat times decreased to: %d", s_NPHCtrlInfo.TreatCounts);
            }
            break;
        default:
            break;
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
                // ????????????2s??
                Drv_IODevice_StartBuzzer(2000);
                break;
            case E_NPH_RUN_STOP:
                LOG_I("NPH state changed to STOP");
                // ????????????2s??
                Drv_IODevice_StartBuzzer(2000);
                break;
            case E_NPH_RUN_WAIT_RETURN:
                LOG_I("NPH state changed to WAIT_RETURN");
                break;
            default:
                break;
        }
    }
}

void App_NegPrsHeat_CheckProbe(void)
{
    static uint16_t debounceCount = 0;
    if(App_TreatMgr_GetProbeStatus() != E_IODEVICE_MODE_NEGATIVE_PRESSURE_HEAT) {
        debounceCount++;
        if(debounceCount >= PROBE_STATUS_DEBOUNCE_CNT) {
            debounceCount = 0;
            s_NPHCtrlInfo.isWaitReturn = true;
        }
    } else {
        debounceCount = 0;
    }

    if(s_NPHCtrlInfo.isWaitReturn) {
        App_NegPrsHeat_ChangeState(E_NPH_RUN_STOP);
    }
}

void App_NegPrsHeat_Monitor(void)
{
    // ???????? mgr ?????
}

bool App_NegPrsHeat_StartCheck()
{
    Heat_TransData_t *pTransData = App_Comm_GetHeatTransData();
    
    // 1. ???????????????????
    if(pTransData->RxWorkState.work_state != WORK_STATE_START) {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 2. ?????????????0??0-3600s??
    if(pTransData->RxWorkState.work_time == 0 || pTransData->RxWorkState.work_time > NPH_WORK_TIME_MAX) {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 3. ???????????10-100KPa??
    if(pTransData->RxWorkState.pressure < NPH_PRESSURE_MIN_KPA || 
       pTransData->RxWorkState.pressure > NPH_PRESSURE_MAX_KPA) {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 4. ?????????????0.1-60s????100ms??
    if(pTransData->RxWorkState.suck_time < (NPH_SUCK_TIME_MIN_MS/100) || 
       pTransData->RxWorkState.suck_time > (NPH_SUCK_TIME_MAX_MS/100)) {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    if(pTransData->RxWorkState.release_time < (NPH_RELEASE_TIME_MIN_MS/100) || 
       pTransData->RxWorkState.release_time > (NPH_RELEASE_TIME_MAX_MS/100)) {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    if(!App_TreatMgr_GetFootSwitchClosed()) {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    if(App_TreatMgr_GetProbeStatus() != E_IODEVICE_MODE_NEGATIVE_PRESSURE_HEAT) {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_PROBE_NOT_CONNECTED;
        return false;
    }
    
    if(s_NPHCtrlInfo.TreatCounts == 0) {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }

    if(s_NPHCtrlInfo.TreatRemainTimes == 0) {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
	
    LOG_I("NPH: Start check passed");
    return true;
}

void App_NegPrsHeat_SetWorkParams(void)
{
    Heat_TransData_t *pTransData = App_Comm_GetHeatTransData();
    
    s_NPHCtrlInfo.WorkTempLimit = pTransData->RxWorkState.temp_limit;
    s_NPHCtrlInfo.Pressure = pTransData->RxWorkState.pressure;
    s_NPHCtrlInfo.SuckTime = pTransData->RxWorkState.suck_time;  // ????100ms
    s_NPHCtrlInfo.ReleaseTime = pTransData->RxWorkState.release_time;  // ????100ms
    
    // ?????????????????
    s_NPHCtrlInfo.targetPressure = App_NegPrsHeat_PressureToVoltage(s_NPHCtrlInfo.Pressure);
    
    // ?????pwr_control1????????
    Drv_IODevice_ChangeChannel(CHANNEL_READY);
    
    // ??????????
    s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_IDLE;
    s_NPHCtrlInfo.motorState = false;
    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 0);
    
    // ?????????
    s_NPHCtrlInfo.heatControlActive = false;
    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 0);
    
    LOG_I("NPH: Work params set - temp_limit=%d, work_time=%d, pressure=%d, suck=%d, release=%d", 
          s_NPHCtrlInfo.WorkTempLimit, s_NPHCtrlInfo.TreatRemainTimes, 
          s_NPHCtrlInfo.Pressure, s_NPHCtrlInfo.SuckTime, s_NPHCtrlInfo.ReleaseTime);
}

bool App_NegPrsHeat_IsHeadTempNormal(void)
{
    uint16_t temp = Drv_ADC_GetRealValue(E_ADC_CHANNEL_HAND_NTC);
    uint32_t currentTime = Drv_Delay_GetTickMs();
    bool isNormal = true;
    
    // ?????????????NTC?????????
    if(temp == 0xFFFF || temp == 0xEEFF)
    {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_TEMP_SENSOR_ERROR;
        LOG_W("NPH: Temperature sensor error: %d", temp);
        isNormal = false;
    }
    // ?????????65??650 * 0.1?C??
    else if(temp > NPH_TEMP_ERROR_THRESHOLD)
    {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_TEMP_TOO_HIGH;
        LOG_W("NPH: Head temperature too high: %d (threshold: %d)", temp, NPH_TEMP_ERROR_THRESHOLD);
        isNormal = false;
    }
    // ???????2s????65??
    if(s_NPHCtrlInfo.lastTemp > 0)
    {
        uint16_t tempRise = temp - s_NPHCtrlInfo.lastTemp;
        uint32_t timeElapsed = currentTime - s_NPHCtrlInfo.tempErrorStartTime;
        
        // ??????????2s??
        if(tempRise > 0 && timeElapsed <= NPH_TEMP_ERROR_TIME_MS)
        {
            // ??????????65??????2s??????????
            if(temp >= NPH_TEMP_ERROR_THRESHOLD)
            {
                s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_TEMP_RISE_TOO_FAST;
                LOG_W("NPH: Temperature rise to 65 degree in %d ms (from %d to %d)", 
                      timeElapsed, s_NPHCtrlInfo.lastTemp, temp);
                isNormal = false;
            }
        }
        
        // ????????????2s???????????????
        if(tempRise <= 0 || timeElapsed > NPH_TEMP_ERROR_TIME_MS)
        {
            s_NPHCtrlInfo.tempErrorStartTime = currentTime;
            s_NPHCtrlInfo.lastTemp = temp;  // ??????
        }
    }
    else
    {
        // ???????????????????????????
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
    
    // ?????????????
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
        // ???????????
        Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 0);
        s_NPHCtrlInfo.heatControlActive = false;
        return;
    }
    
    // ???????????????????????????????????
    // ??????????hysteresis?????????
    if(temp < targetTemp - 5)  // ????????5*0.1???????
    {
        needHeat = true;
    }
    else if(temp > targetTemp + 5)  // ????????5*0.1?????????
    {
        needHeat = false;
    }
    else
    {
        // ??????????????
        needHeat = s_NPHCtrlInfo.heatControlActive;
    }
    
    // ??CTR_HEAT_HP
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
            // ?????
            s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_SUCKING;
            s_NPHCtrlInfo.vacuumStateStartTime = currentTime;
            s_NPHCtrlInfo.suckStartTime = currentTime;
            s_NPHCtrlInfo.motorState = true;
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 1);
            LOG_I("NPH: Start sucking, target pressure: %d KPa", s_NPHCtrlInfo.Pressure);
            break;
            
        case E_NPH_VACUUM_STATE_SUCKING:
            // ?????????????????????????currentPressure????????targetPressure??
            // ????????????????????????
            // ??ADC?????????????????currentPressure >= targetPressure
            // ??ADC??????????????????????????
            targetVoltage = App_NegPrsHeat_PressureToVoltage(s_NPHCtrlInfo.Pressure);
            if(pressureVoltage >= targetVoltage)
            {
                // ???????????????
                s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_MAINTAIN;
                s_NPHCtrlInfo.maintainStartTime = currentTime;
                LOG_I("NPH: Target pressure reached (%d KPa), start maintaining", s_NPHCtrlInfo.Pressure);
            }
            else
            {
                // ????????????
                if(!s_NPHCtrlInfo.motorState)
                {
                    s_NPHCtrlInfo.motorState = true;
                    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 1);
                }
            }
            break;
            
        case E_NPH_VACUUM_STATE_MAINTAIN:
            // ??????
            // ???????????
            maintainElapsed = currentTime - s_NPHCtrlInfo.maintainStartTime;
            maintainTimeMs = s_NPHCtrlInfo.SuckTime * 100;  // ??????????
            
            if(maintainElapsed >= maintainTimeMs)
            {
                // ???????????
                s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_RELEASING;
                s_NPHCtrlInfo.releaseStartTime = currentTime;
                s_NPHCtrlInfo.motorState = false;
                Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 0);
                // ??????????????????CTR_HP_lose??
                Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_LOSE, 1);
                LOG_I("NPH: Maintain time reached, start releasing");
            }
            else
            {
				
                // ?????????????????
                targetVoltage = App_NegPrsHeat_PressureToVoltage(s_NPHCtrlInfo.Pressure);
//                voltageDiff = (pressureVoltage > targetVoltage) ? 
//                                       (pressureVoltage - targetVoltage) : 
//                                       (targetVoltage - pressureVoltage);
                uint16_t thresholdVoltage = App_NegPrsHeat_PressureToVoltage(5);  // 5KPa??????
                
                if(pressureVoltage < targetVoltage - thresholdVoltage)  // ????????????
                {
                    // ????????????
                    if(!s_NPHCtrlInfo.motorState)
                    {
                        s_NPHCtrlInfo.motorState = true;
                        Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 1);
                    }
                }
                else if(pressureVoltage > targetVoltage + thresholdVoltage)  // ????????????
                {
                    // ??????????
                    if(s_NPHCtrlInfo.motorState)
                    {
                        s_NPHCtrlInfo.motorState = false;
                        Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 0);
                    }
                }
            }
            break;
        	
        case E_NPH_VACUUM_STATE_RELEASING:
            // ?????
            releaseElapsed = currentTime - s_NPHCtrlInfo.releaseStartTime;
            releaseTimeMs = s_NPHCtrlInfo.ReleaseTime * 100;  // ??????????
            
            if(releaseElapsed >= releaseTimeMs)
            {
                // ??????????????????????
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
    App_NegPrsHeat_WorkTimeHandle();
    App_NegPrsHeat_Monitor();
    App_NegPrsHeat_CheckProbe();
    // Handle the negative pressure heat state
    switch(s_NPHCtrlInfo.runState)
    {
        case E_NPH_RUN_INIT:
            // ????????
            s_NPHCtrlInfo.TreatCountsState = E_TREAT_TIMES_POWER_ON;
            if(App_Memory_LoadNPHParams(&s_NPHCtrlInfo.TreatParams)) {
                s_NPHCtrlInfo.TempLimit = s_NPHCtrlInfo.TreatParams.TempLimit;
                s_NPHCtrlInfo.TreatCounts = s_NPHCtrlInfo.TreatParams.TreatRemainTimes;
                s_NPHCtrlInfo.PreheatEnable = (s_NPHCtrlInfo.TreatParams.PreheatEnable == 1);
                s_NPHCtrlInfo.PreheatTempLimit = s_NPHCtrlInfo.TreatParams.PreheatTempLimit;
                s_NPHCtrlInfo.PreheatTime = s_NPHCtrlInfo.TreatParams.PreheatTime;
                
                LOG_I("NPH: Parameters loaded - temp_limit=%d, remain_times=%d, preheat_enable=%d, preheat_temp=%d, preheat_time=%d",
                      s_NPHCtrlInfo.TempLimit, s_NPHCtrlInfo.TreatCounts,
                      s_NPHCtrlInfo.PreheatEnable, s_NPHCtrlInfo.PreheatTempLimit, s_NPHCtrlInfo.PreheatTime);
            } else {
                LOG_E("NPH: Failed to load parameters");
                s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_READ_PARAMS_FAILED;
            }
            App_NegPrsHeat_ChangeState(E_NPH_RUN_IDLE);
            break;
            
        case E_NPH_RUN_IDLE:
            // ??App_NegPrsHeat_StartCheck??????????
            if(App_NegPrsHeat_StartCheck()) {
                // ??????
                App_NegPrsHeat_SetWorkParams();
                
                // ?????????????????????
                if(s_NPHCtrlInfo.PreheatEnable)
                {
                    App_NegPrsHeat_ChangeState(E_NPH_RUN_PREHEAT);
                }
                else
                {
                    // ?????????
                    Drv_IODevice_ChangeChannel(CHANNEL_NH);
                    App_NegPrsHeat_ChangeState(E_NPH_RUN_WORKING);
                }
            }
            break;
            
        case E_NPH_RUN_PREHEAT:
            // ????????????????
            // ????????????????
            if(s_NPHCtrlInfo.HeadTemp >= s_NPHCtrlInfo.PreheatTempLimit)
            {
                // ????????????
                Drv_IODevice_ChangeChannel(CHANNEL_NH);
                App_NegPrsHeat_ChangeState(E_NPH_RUN_WORKING);
                LOG_I("NPH: Preheat completed, entering working state");
            }
            // ???????
            else if(App_NegPrsHeat_StartCheck() == false)
            {
                App_NegPrsHeat_ChangeState(E_NPH_RUN_STOP);
            }
            else
            {
                // ??????10ms????
                if(Drv_Timer_Tick(&TempMonitorTimer, NPH_TEMP_MONITOR_PERIOD_MS)) {
                    if(App_NegPrsHeat_IsHeadTempNormal() == false) {
                        // ????????????
                        App_NegPrsHeat_ChangeState(E_NPH_RUN_STOP);
                    } else {
                        // ????
                        App_NegPrsHeat_ControlTemperature();
                    }
                }
            }
            break;
            
        case E_NPH_RUN_WORKING:
            
            // ???????
            if(App_NegPrsHeat_StartCheck() == false || 
               s_NPHCtrlInfo.TreatRemainTimes == 0){
                App_NegPrsHeat_ChangeState(E_NPH_RUN_STOP);
            } else {
                // ??????10ms????
                if(Drv_Timer_Tick(&TempMonitorTimer, NPH_TEMP_MONITOR_PERIOD_MS)) {
                    if(App_NegPrsHeat_IsHeadTempNormal() == false) {
                        // ????????????
                        App_NegPrsHeat_ChangeState(E_NPH_RUN_STOP);
                    } else {
                        // ????
                        App_NegPrsHeat_ControlTemperature();
                    }
                }
                
                // ??????
                App_NegPrsHeat_ProcessVacuum();
                /* TreatRemainTimes ? WorkTimeHandle ????? */
            }
            break;
            
        case E_NPH_RUN_STOP:
            // ????
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 0);
            s_NPHCtrlInfo.heatControlActive = false;
            
            // ??????
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 0);
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_LOSE, 0);
            s_NPHCtrlInfo.motorState = false;
            s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_IDLE;
            
            // ??????
            Drv_IODevice_ChangeChannel(CHANNEL_CLOSE);
            if(s_NPHCtrlInfo.isWaitReturn) {
                App_NegPrsHeat_ChangeState(E_NPH_RUN_WAIT_RETURN);
                LOG_I("NPH: Wait return");
                s_NPHCtrlInfo.isWaitReturn = false;
            }
            break;

        case E_NPH_RUN_WAIT_RETURN:
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
    // ???????????
    memset(&s_NPHCtrlInfo, 0, sizeof(NPH_CtrlInfo_t));
    
    // ?????????
    s_NPHCtrlInfo.runState = E_NPH_RUN_INIT;
    s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_IDLE;
    s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_NONE;
    s_NPHCtrlInfo.TreatRemainTimes = 0;
    s_NPHCtrlInfo.TreatCounts = 0;
    s_NPHCtrlInfo.heatControlActive = false;
    s_NPHCtrlInfo.motorState = false;
    
    // ??????????????
    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 0);
    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 0);
    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_LOSE, 0);
    
    LOG_I("Negative Pressure Heat module initialized");
}

Heat_GetStatus_Reply_t *App_NegPrsHeat_GetStatus(void)
{
    return &s_NPHCtrlInfo.Trans.TxStatus;
}

NPH_RunState_EnumDef App_NegPrsHeat_GetRunState(void)
{
    return s_NPHCtrlInfo.runState;
}

/**************************End of file********************************/
