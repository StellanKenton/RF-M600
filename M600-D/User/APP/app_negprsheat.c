/***********************************************************************************
* @file     : app_negprsheat.c
* @brief    : Negative Pressure Heat treatment module implementation
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2025
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
    // Update work state
    if(s_NPHCtrlInfo.runState == E_NPH_RUN_WORKING) {
        s_NPHCtrlInfo.Trans.TxStatus.work_state = 0x01;
    } else if(s_NPHCtrlInfo.runState == E_NPH_RUN_PREHEAT) {
        s_NPHCtrlInfo.Trans.TxStatus.preheat_state = 0x01;
        s_NPHCtrlInfo.Trans.TxStatus.work_state = 0x00;
    } else {
        s_NPHCtrlInfo.Trans.TxStatus.work_state = 0x00;
        s_NPHCtrlInfo.Trans.TxStatus.preheat_state = 0x00;
    }
    
    s_NPHCtrlInfo.Trans.TxStatus.temp_limit = s_NPHCtrlInfo.WorkTempLimit;
    s_NPHCtrlInfo.Trans.TxStatus.remain_heat_time = s_NPHCtrlInfo.TreatCounts / 100;  /* 10ms -> s */
    s_NPHCtrlInfo.Trans.TxStatus.suck_time = s_NPHCtrlInfo.SuckTime;
    s_NPHCtrlInfo.Trans.TxStatus.release_time = s_NPHCtrlInfo.ReleaseTime;
    s_NPHCtrlInfo.Trans.TxStatus.pressure = s_NPHCtrlInfo.Pressure;
    s_NPHCtrlInfo.Trans.TxStatus.head_temp = s_NPHCtrlInfo.HeadTemp;
    s_NPHCtrlInfo.Trans.TxStatus.preheat_temp_limit = s_NPHCtrlInfo.PreheatTempLimit;
    s_NPHCtrlInfo.Trans.TxStatus.remain_preheat_time = s_NPHCtrlInfo.PreheatTime;
    
    /* Get probe/foot state from treatmgr and update conn_state */
    bool headConnected = (App_TreatMgr_GetProbeStatus() == E_IODEVICE_MODE_NEGATIVE_PRESSURE_HEAT);
    bool footClosed = App_TreatMgr_GetFootSwitchClosed();
    if (headConnected && footClosed) {
        s_NPHCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_CONNECTED_FOOT_CLOSED;
    } else if (!headConnected && footClosed) {
        s_NPHCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_DISCONNECTED_FOOT_CLOSED;
    } else if (headConnected && !footClosed) {
        s_NPHCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_CONNECTED_FOOT_OPEN;
    } else {
        s_NPHCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_DISCONNECTED_FOOT_OPEN;
    }
    s_NPHCtrlInfo.Trans.TxStatus.error_code = s_NPHCtrlInfo.ErrorCode;
}

void App_NegPrsHeat_RxDataHandle(void)
{
    Heat_TransData_t *pTransData = App_Comm_GetHeatTransData();
    
    /* RxWorkState and WorkTimeHandle are processed in WorkTimeHandle() */
    s_NPHCtrlInfo.Trans.RxWorkState = pTransData->RxWorkState;
    s_NPHCtrlInfo.Trans.RxConfig = pTransData->RxConfig;
    /* Config flag set: apply RxPreheat and save params */
    if(pTransData->flag.bits.Rely_Config)
    {
        /* RxPreheat requests preheat: apply temp_limit and work_time */
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
        
        /* Save preheat params to NPH and memory */
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
    static uint8_t s_lastRxWorkState = 0x00;

    if(s_NPHCtrlInfo.Trans.RxWorkState.work_state != s_lastRxWorkState) {
        if (s_NPHCtrlInfo.Trans.RxWorkState.work_state == WORK_STATE_RESET && s_lastRxWorkState != WORK_STATE_RESET) {
            s_NPHCtrlInfo.TreatCountsState = E_TREAT_TIMES_RESET;
        }
        s_lastRxWorkState = s_NPHCtrlInfo.Trans.RxWorkState.work_state;
    }

    switch(s_NPHCtrlInfo.TreatCountsState)
    {
        case E_TREAT_TIMES_POWER_ON:
            if(s_NPHCtrlInfo.Trans.RxWorkState.work_time > 0 && s_NPHCtrlInfo.TreatRemainTimes > 0)
            {
                s_NPHCtrlInfo.TreatCounts = s_NPHCtrlInfo.Trans.RxWorkState.work_time * 1000;  /* s -> 10ms */
                s_NPHCtrlInfo.WorkTempLimit = s_NPHCtrlInfo.Trans.RxWorkState.temp_limit;
                s_NPHCtrlInfo.Pressure = s_NPHCtrlInfo.Trans.RxWorkState.pressure;
                s_NPHCtrlInfo.SuckTime = s_NPHCtrlInfo.Trans.RxWorkState.suck_time;
                s_NPHCtrlInfo.ReleaseTime = s_NPHCtrlInfo.Trans.RxWorkState.release_time;
                s_NPHCtrlInfo.TreatCountsState = E_TREAT_TIMES_WORKING;
                s_NPHCtrlInfo.TreatParams.TreatRemainTimes = s_NPHCtrlInfo.TreatRemainTimes - 1;
                s_NPHCtrlInfo.TreatRemainTimes--;
                App_Memory_SaveNPHParams(&s_NPHCtrlInfo.TreatParams);
                LOG_I("NPH: Remaining treat times decreased to: %d", s_NPHCtrlInfo.TreatRemainTimes);
            }
            break;
        case E_TREAT_TIMES_RESET:
            if(s_NPHCtrlInfo.Trans.RxWorkState.work_time > 0 && s_NPHCtrlInfo.TreatRemainTimes > 0)
            {
                s_NPHCtrlInfo.TreatCounts = s_NPHCtrlInfo.Trans.RxWorkState.work_time * 1000;  /* s -> 10ms */
                s_NPHCtrlInfo.WorkTempLimit = s_NPHCtrlInfo.Trans.RxWorkState.temp_limit;
                s_NPHCtrlInfo.Pressure = s_NPHCtrlInfo.Trans.RxWorkState.pressure;
                s_NPHCtrlInfo.SuckTime = s_NPHCtrlInfo.Trans.RxWorkState.suck_time;
                s_NPHCtrlInfo.ReleaseTime = s_NPHCtrlInfo.Trans.RxWorkState.release_time;
                s_NPHCtrlInfo.TreatCountsState = E_TREAT_TIMES_WORKING;
                s_NPHCtrlInfo.TreatParams.TreatRemainTimes = s_NPHCtrlInfo.TreatRemainTimes - 1;
                s_NPHCtrlInfo.TreatRemainTimes--;
                App_Memory_SaveNPHParams(&s_NPHCtrlInfo.TreatParams);
                LOG_I("NPH: Remaining treat times decreased to: %d", s_NPHCtrlInfo.TreatRemainTimes);
            }
            break;
        case E_TREAT_TIMES_WORKING:
            if(s_NPHCtrlInfo.TreatCounts > 0 && s_NPHCtrlInfo.runState == E_NPH_RUN_WORKING)
            {
                if(s_NPHCtrlInfo.TreatCounts >= TREAT_TASK_TIME) {
                    s_NPHCtrlInfo.TreatCounts -= TREAT_TASK_TIME;
                } else {
                    s_NPHCtrlInfo.TreatCounts = 0;
                }
            }
            if(s_NPHCtrlInfo.TreatCounts == 0)
            {
                s_NPHCtrlInfo.TreatCountsState = E_TREAT_TIMES_WAIT;
            }
            break;
        case E_TREAT_TIMES_WAIT:
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
                /* Working state: beep 2s */
                Drv_IODevice_StartBuzzer(2000);
                break;
            case E_NPH_RUN_STOP:
                LOG_I("NPH state changed to STOP");
                /* Stop state: beep 2s */
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
    /* Monitor logic (reserved for treatmgr integration) */
}

bool App_NegPrsHeat_StartCheck()
{
    /* 1. work_state must be START */
    if(s_NPHCtrlInfo.Trans.RxWorkState.work_state != WORK_STATE_START) {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    /* 2. work_time valid: non-zero, 0~3600s */
    if(s_NPHCtrlInfo.Trans.RxWorkState.work_time == 0 || s_NPHCtrlInfo.Trans.RxWorkState.work_time > NPH_WORK_TIME_MAX) {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    /* 3. pressure in 10~100 KPa */
    if(s_NPHCtrlInfo.Trans.RxWorkState.pressure < NPH_PRESSURE_MIN_KPA || 
       s_NPHCtrlInfo.Trans.RxWorkState.pressure > NPH_PRESSURE_MAX_KPA) {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    /* 4. suck/release time 0.1~60s, unit 100ms */
    if(s_NPHCtrlInfo.Trans.RxWorkState.suck_time < (NPH_SUCK_TIME_MIN_MS/100) || 
       s_NPHCtrlInfo.Trans.RxWorkState.suck_time > (NPH_SUCK_TIME_MAX_MS/100)) {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
    
    if(s_NPHCtrlInfo.Trans.RxWorkState.release_time < (NPH_RELEASE_TIME_MIN_MS/100) || 
       s_NPHCtrlInfo.Trans.RxWorkState.release_time > (NPH_RELEASE_TIME_MAX_MS/100)) {
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
    
    if(s_NPHCtrlInfo.TreatRemainTimes == 0) {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }

    if(s_NPHCtrlInfo.TreatCounts == 0) {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_INVALID_PARAMS;
        return false;
    }
	
    LOG_I("NPH: Start check passed");
    return true;
}

void App_NegPrsHeat_SetWorkParams(void)
{
    s_NPHCtrlInfo.WorkTempLimit = s_NPHCtrlInfo.Trans.RxWorkState.temp_limit;
    s_NPHCtrlInfo.Pressure = s_NPHCtrlInfo.Trans.RxWorkState.pressure;
    s_NPHCtrlInfo.SuckTime = s_NPHCtrlInfo.Trans.RxWorkState.suck_time;  /* unit: 100ms */
    s_NPHCtrlInfo.ReleaseTime = s_NPHCtrlInfo.Trans.RxWorkState.release_time;  /* unit: 100ms */
    
    /* Convert target pressure to ADC voltage */
    s_NPHCtrlInfo.targetPressure = App_NegPrsHeat_PressureToVoltage(s_NPHCtrlInfo.Pressure);
    
    /* Switch to pwr_control1 / CHANNEL_READY */
    Drv_IODevice_ChangeChannel(CHANNEL_READY);
    
    /* Init vacuum and motor off */
    s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_IDLE;
    s_NPHCtrlInfo.motorState = false;
    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 0);
    
    /* Init heat control off */
    s_NPHCtrlInfo.heatControlActive = false;
    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 0);
    
    LOG_I("NPH: Work params set - temp_limit=%d, work_time=%d, pressure=%d, suck=%d, release=%d", 
          s_NPHCtrlInfo.WorkTempLimit, s_NPHCtrlInfo.TreatCounts, 
          s_NPHCtrlInfo.Pressure, s_NPHCtrlInfo.SuckTime, s_NPHCtrlInfo.ReleaseTime);
}

bool App_NegPrsHeat_IsHeadTempNormal(void)
{
    uint16_t temp = Drv_ADC_GetRealValue(E_ADC_CHANNEL_HAND_NTC);
    uint32_t currentTime = Drv_Delay_GetTickMs();
    bool isNormal = true;
    
    /* Invalid NTC read (sensor error) */
    if(temp == 0xFFFF || temp == 0xEEFF)
    {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_TEMP_SENSOR_ERROR;
        LOG_W("NPH: Temperature sensor error: %d", temp);
        isNormal = false;
    }
    /* Over 65C (650 * 0.1C) */
    else if(temp > NPH_TEMP_ERROR_THRESHOLD)
    {
        s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_TEMP_TOO_HIGH;
        LOG_W("NPH: Head temperature too high: %d (threshold: %d)", temp, NPH_TEMP_ERROR_THRESHOLD);
        isNormal = false;
    }
    /* Rise rate check: reach 65C within 2s */
    if(s_NPHCtrlInfo.lastTemp > 0)
    {
        uint16_t tempRise = temp - s_NPHCtrlInfo.lastTemp;
        uint32_t timeElapsed = currentTime - s_NPHCtrlInfo.tempErrorStartTime;
        
        /* Within 2s */
        if(tempRise > 0 && timeElapsed <= NPH_TEMP_ERROR_TIME_MS)
        {
            /* Reached 65C within 2s -> error */
            if(temp >= NPH_TEMP_ERROR_THRESHOLD)
            {
                s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_TEMP_RISE_TOO_FAST;
                LOG_W("NPH: Temperature rise to 65 degree in %d ms (from %d to %d)", 
                      timeElapsed, s_NPHCtrlInfo.lastTemp, temp);
                isNormal = false;
            }
        }
        
        /* Reset timing if not rising or >2s elapsed */
        if(tempRise <= 0 || timeElapsed > NPH_TEMP_ERROR_TIME_MS)
        {
            s_NPHCtrlInfo.tempErrorStartTime = currentTime;
            s_NPHCtrlInfo.lastTemp = temp;  /* Update baseline */
        }
    }
    else
    {
        /* First sample: record time and temp */
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
    
    /* Get target temp by run state */
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
        /* Other state: heat off */
        Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 0);
        s_NPHCtrlInfo.heatControlActive = false;
        return;
    }
    
    /* Hysteresis: below target-5 (0.5C) heat on, above target+5 heat off */
    if(temp < targetTemp - 5)  /* Below target-5: heat on */
    {
        needHeat = true;
    }
    else if(temp > targetTemp + 5)  /* Above target+5: heat off */
    {
        needHeat = false;
    }
    else
    {
        /* In band: keep last heat state */
        needHeat = s_NPHCtrlInfo.heatControlActive;
    }
    
    /* Set CTR_HEAT_HP */
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
            /* Start sucking */
            s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_SUCKING;
            s_NPHCtrlInfo.vacuumStateStartTime = currentTime;
            s_NPHCtrlInfo.suckStartTime = currentTime;
            s_NPHCtrlInfo.motorState = true;
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 1);
            LOG_I("NPH: Start sucking, target pressure: %d KPa", s_NPHCtrlInfo.Pressure);
            break;
            
        case E_NPH_VACUUM_STATE_SUCKING:
            /* Compare ADC pressure voltage with target; when currentPressure >= target, go to maintain */
            targetVoltage = App_NegPrsHeat_PressureToVoltage(s_NPHCtrlInfo.Pressure);
            if(pressureVoltage >= targetVoltage)
            {
                /* Target reached, enter maintain */
                s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_MAINTAIN;
                s_NPHCtrlInfo.maintainStartTime = currentTime;
                LOG_I("NPH: Target pressure reached (%d KPa), start maintaining", s_NPHCtrlInfo.Pressure);
            }
            else
            {
                /* Keep motor on until target */
                if(!s_NPHCtrlInfo.motorState)
                {
                    s_NPHCtrlInfo.motorState = true;
                    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 1);
                }
            }
            break;
            
        case E_NPH_VACUUM_STATE_MAINTAIN:
            /* Maintain time: SuckTime in 100ms units -> ms */
            maintainElapsed = currentTime - s_NPHCtrlInfo.maintainStartTime;
            maintainTimeMs = s_NPHCtrlInfo.SuckTime * 100;  /* 100ms -> ms */
            
            if(maintainElapsed >= maintainTimeMs)
            {
                /* Maintain done, start release */
                s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_RELEASING;
                s_NPHCtrlInfo.releaseStartTime = currentTime;
                s_NPHCtrlInfo.motorState = false;
                Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 0);
                /* Open release valve: set CTR_HP_LOSE */
                Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_LOSE, 1);
                LOG_I("NPH: Maintain time reached, start releasing");
            }
            else
            {
                /* Maintain pressure: re-suck if below target, stop if above */
                targetVoltage = App_NegPrsHeat_PressureToVoltage(s_NPHCtrlInfo.Pressure);
//                voltageDiff = (pressureVoltage > targetVoltage) ? 
//                                       (pressureVoltage - targetVoltage) : 
//                                       (targetVoltage - pressureVoltage);
                uint16_t thresholdVoltage = App_NegPrsHeat_PressureToVoltage(5);  /* 5KPa threshold */
                
                if(pressureVoltage < targetVoltage - thresholdVoltage)  /* Below target: motor on */
                {
                    if(!s_NPHCtrlInfo.motorState)
                    {
                        s_NPHCtrlInfo.motorState = true;
                        Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 1);
                    }
                }
                else if(pressureVoltage > targetVoltage + thresholdVoltage)  /* Above target: motor off */
                {
                    if(s_NPHCtrlInfo.motorState)
                    {
                        s_NPHCtrlInfo.motorState = false;
                        Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 0);
                    }
                }
            }
            break;
        	
        	case E_NPH_VACUUM_STATE_RELEASING:
            /* Release time: ReleaseTime in 100ms -> ms */
            releaseElapsed = currentTime - s_NPHCtrlInfo.releaseStartTime;
            releaseTimeMs = s_NPHCtrlInfo.ReleaseTime * 100;  /* 100ms -> ms */
            
            if(releaseElapsed >= releaseTimeMs)
            {
                /* Release done, close valve, ready for next cycle */
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
            /* Load params and go to IDLE */
            s_NPHCtrlInfo.TreatCountsState = E_TREAT_TIMES_POWER_ON;
            if(App_Memory_LoadNPHParams(&s_NPHCtrlInfo.TreatParams)) {
                s_NPHCtrlInfo.TempLimit = s_NPHCtrlInfo.TreatParams.TempLimit;
                s_NPHCtrlInfo.TreatRemainTimes = s_NPHCtrlInfo.TreatParams.TreatRemainTimes;
                s_NPHCtrlInfo.PreheatEnable = (s_NPHCtrlInfo.TreatParams.PreheatEnable == 1);
                s_NPHCtrlInfo.PreheatTempLimit = s_NPHCtrlInfo.TreatParams.PreheatTempLimit;
                s_NPHCtrlInfo.PreheatTime = s_NPHCtrlInfo.TreatParams.PreheatTime;
                
                LOG_I("NPH: Parameters loaded - temp_limit=%d, remain_times=%d, preheat_enable=%d, preheat_temp=%d, preheat_time=%d",
                      s_NPHCtrlInfo.TempLimit, s_NPHCtrlInfo.TreatRemainTimes,
                      s_NPHCtrlInfo.PreheatEnable, s_NPHCtrlInfo.PreheatTempLimit, s_NPHCtrlInfo.PreheatTime);
            } else {
                LOG_E("NPH: Failed to load parameters");
                s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_READ_PARAMS_FAILED;
            }
            App_NegPrsHeat_ChangeState(E_NPH_RUN_IDLE);
            break;
            
        case E_NPH_RUN_IDLE:
            /* If StartCheck passes, set params and go PREHEAT or WORKING */
            if(App_NegPrsHeat_StartCheck()) {
                App_NegPrsHeat_SetWorkParams();
                
                if(s_NPHCtrlInfo.PreheatEnable)
                {
                    App_NegPrsHeat_ChangeState(E_NPH_RUN_PREHEAT);
                }
                else
                {
                    /* No preheat: switch to NH and WORKING */
                    Drv_IODevice_ChangeChannel(CHANNEL_NH);
                    App_NegPrsHeat_ChangeState(E_NPH_RUN_WORKING);
                }
            }
            break;
            
        case E_NPH_RUN_PREHEAT:
            /* When preheat temp reached, switch to NH and WORKING */
            if(s_NPHCtrlInfo.HeadTemp >= s_NPHCtrlInfo.PreheatTempLimit)
            {
                Drv_IODevice_ChangeChannel(CHANNEL_NH);
                App_NegPrsHeat_ChangeState(E_NPH_RUN_WORKING);
                LOG_I("NPH: Preheat completed, entering working state");
            }
            else if(App_NegPrsHeat_StartCheck() == false)
            {
                App_NegPrsHeat_ChangeState(E_NPH_RUN_STOP);
            }
            else
            {
                /* Every 10ms: temp monitor and heat control */
                if(Drv_Timer_Tick(&TempMonitorTimer, NPH_TEMP_MONITOR_PERIOD_MS)) {
                    if(App_NegPrsHeat_IsHeadTempNormal() == false) {
                        App_NegPrsHeat_ChangeState(E_NPH_RUN_STOP);
                    } else {
                        App_NegPrsHeat_ControlTemperature();
                    }
                }
            }
            break;
            
        case E_NPH_RUN_WORKING:
            /* StartCheck fail or remain time 0 -> STOP */
            if(App_NegPrsHeat_StartCheck() == false ||
               s_NPHCtrlInfo.TreatCounts == 0){
                App_NegPrsHeat_ChangeState(E_NPH_RUN_STOP);
            } else {
                /* Every 10ms: temp monitor and heat control */
                if(Drv_Timer_Tick(&TempMonitorTimer, NPH_TEMP_MONITOR_PERIOD_MS)) {
                    if(App_NegPrsHeat_IsHeadTempNormal() == false) {
                        App_NegPrsHeat_ChangeState(E_NPH_RUN_STOP);
                    } else {
                        App_NegPrsHeat_ControlTemperature();
                    }
                }
                
                App_NegPrsHeat_ProcessVacuum();
                /* TreatCounts decremented in WorkTimeHandle */
            }
            break;
            
        case E_NPH_RUN_STOP:
            /* Heat off */
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 0);
            s_NPHCtrlInfo.heatControlActive = false;
            
            /* Motor and release valve off */
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_MOTOR, 0);
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HP_LOSE, 0);
            s_NPHCtrlInfo.motorState = false;
            s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_IDLE;
            
            /* Switch channel close */
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
    /* Clear control info */
    memset(&s_NPHCtrlInfo, 0, sizeof(NPH_CtrlInfo_t));
    
    /* Default state and error */
    s_NPHCtrlInfo.runState = E_NPH_RUN_INIT;
    s_NPHCtrlInfo.vacuumState = E_NPH_VACUUM_STATE_IDLE;
    s_NPHCtrlInfo.ErrorCode = E_NPH_ERROR_NONE;
    s_NPHCtrlInfo.TreatRemainTimes = 0;
    s_NPHCtrlInfo.TreatCounts = 0;
    s_NPHCtrlInfo.heatControlActive = false;
    s_NPHCtrlInfo.motorState = false;
    
    /* GPIO outputs off */
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
