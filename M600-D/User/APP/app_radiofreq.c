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
 * @brief Calculate voltage from work level (1-20????11-30V)
 * @param level Work level (0-20)
 * @retval Voltage in mV
 */
static uint16_t App_RadioFreq_CalculateVoltage(uint8_t level)
{
    if(level == 0) {
        return RF_VOLTAGE_INIT_MV;  // ??0??7V
    }
    if(level > RF_WORK_LEVEL_MAX) {
        level = RF_WORK_LEVEL_MAX;
    }
    // 1?????11V??20?????30V
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
    s_RFCtrlInfo.Trans.TxStatus.remain_time = s_RFCtrlInfo.TreatRemainTimes;
    s_RFCtrlInfo.Trans.TxStatus.work_level = s_RFCtrlInfo.WorkLevel;
    s_RFCtrlInfo.Trans.TxStatus.head_temp = s_RFCtrlInfo.HeadTemp;
    
    // ???????? mgr ??
    bool headConnected = (App_TreatMgr_GetProbeStatus() == E_IODEVICE_MODE_RADIO_FREQUENCY);
    bool footClosed = App_TreatMgr_GetFootSwitchClosed();
    if (headConnected && footClosed) {
        s_RFCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_CONNECTED_FOOT_CLOSED;
    } else if (!headConnected && footClosed) {
        s_RFCtrlInfo.Trans.TxStatus.conn_state = CONN_STATE_DISCONNECTED_FOOT_CLOSED;
    } else if (headConnected && !footClosed) {
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
        // ??????
        // ????????????
        s_RFCtrlInfo.TreatRemainTimes = pTransData->RxWorkState.work_time;
        s_RFCtrlInfo.WorkLevel = pTransData->RxWorkState.work_level;
        
        // ??????????
        if(s_RFCtrlInfo.TreatCounts > 0)
        {
            s_RFCtrlInfo.TreatCounts--;
            // ??????
            s_RFCtrlInfo.TreatParams.TreatRemainTimes = s_RFCtrlInfo.TreatCounts;
            App_Memory_SaveRFParams(&s_RFCtrlInfo.TreatParams);
            LOG_I("RF Reset: Remaining treat times decreased to: %d", s_RFCtrlInfo.TreatCounts);
        }
        
        LOG_I("RF Reset: Work time=%d, Work level=%d", s_RFCtrlInfo.TreatRemainTimes, s_RFCtrlInfo.WorkLevel);
    }
    
    // ??????
    if(pTransData->flag.bits.Rely_Config)
    {
        s_RFCtrlInfo.TempLimit = pTransData->RxConfig.temp_limit;
        // ??????
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
                // ????????????2s??
                Drv_IODevice_StartBuzzer(2000);
                break;
            case E_RF_RUN_STOP:
                LOG_I("RF state changed to STOP");
                // ????????????2s??
                Drv_IODevice_StartBuzzer(2000);
                break;
            default:
                break;
        }
    }
}

void App_RadioFreq_Monitor(void)
{
    // ???????? mgr ?????
}

bool App_RadioFreq_StartCheck()
{
    RF_TransData_t *pTransData = App_Comm_GetRFTransData();
    
    // 1. ????????????????????
    if(pTransData->RxWorkState.work_state != WORK_STATE_START) {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 2. ?????????????0??0-3600s??
    if(pTransData->RxWorkState.work_time == 0 || pTransData->RxWorkState.work_time > 3600) {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 3. ???????????0??0-20??
    if(pTransData->RxWorkState.work_level == 0 || pTransData->RxWorkState.work_level > RF_WORK_LEVEL_MAX) {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 4. ???????????
    if(!App_TreatMgr_GetFootSwitchClosed()) {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // 5. ????????????????
    if(App_TreatMgr_GetProbeStatus() != E_IODEVICE_MODE_RADIO_FREQUENCY) {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_PROBE_NOT_CONNECTED;
        return false;
    }
    
    // 6. ?????????????
    if(s_RFCtrlInfo.TreatCounts == 0) {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_INVALID_PARAMS;
        return false;
    }
    
    // ????????
    LOG_I("RF: Start check passed");
    return true;
}

void App_RadioFreq_SetWorkParams(void)
{
    RF_TransData_t *pTransData = App_Comm_GetRFTransData();
    
    // ??????
    s_RFCtrlInfo.WorkLevel = pTransData->RxWorkState.work_level;
    s_RFCtrlInfo.TreatRemainTimes = pTransData->RxWorkState.work_time;
    
    // ????????????????
    s_RFCtrlInfo.VoltageTarget = App_RadioFreq_CalculateVoltage(s_RFCtrlInfo.WorkLevel);
    
    // ???????????7V
    s_RFCtrlInfo.Voltage = RF_VOLTAGE_INIT_MV;
    Drv_DAC_SetVoltage(s_RFCtrlInfo.Voltage);
    
    // ??SI5351??1MHz??PWM????????
    // ??????????????????100ns
    Drv_SI5351_SetComplementaryPWM(RF_FREQUENCY_KHZ, 100);
    
    // ?????pwr_control1?????
    Drv_IODevice_ChangeChannel(CHANNEL_READY);
    
    // CTR_HEAT_HP?????????????
    Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 1);
    
    LOG_I("RF: Work params set - level=%d, time=%d, voltage_target=%d", 
          s_RFCtrlInfo.WorkLevel, s_RFCtrlInfo.TreatRemainTimes, s_RFCtrlInfo.VoltageTarget);
}

bool App_RadioFreq_IsCurrentNormal(void)
{
    uint16_t current = Drv_ADC_GetRealValue(E_ADC_CHANNEL_RF_I);
    uint16_t currentVoltage = Drv_DAC_GetVoltage();
    uint16_t newVoltage = currentVoltage;
    bool isNormal = true;
    
    if(current < RF_CURRENT_THRESHOLD_MV)
    {
        // ?????????0.5V????????7V
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
        // ????????????????????????????????
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
        // ??????????????????7V
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
    // TODO: ??????????????RF_TX/RF_RX??
    // ?????????????????????????
    // ????????????????s_RFCtrlInfo.HeadTemp??
    
    bool isNormal = true;
    
    if(s_RFCtrlInfo.HeadTemp > s_RFCtrlInfo.TempLimit)
    {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_TEMP_TOO_HIGH;
        LOG_W("RF: Head temperature too high: %d (limit: %d)", 
              s_RFCtrlInfo.HeadTemp, s_RFCtrlInfo.TempLimit);
        // ????????????
        isNormal = false;
    }
    else
    {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_NONE;
    }
    
    return isNormal;
}

void App_RadioFreq_CheckProbe(void)
{
    static uint16_t debounceCount = 0;
    if(App_TreatMgr_GetProbeStatus() != E_IODEVICE_MODE_RADIO_FREQUENCY) {
        debounceCount++;
        if(debounceCount >= PROBE_STATUS_DEBOUNCE_CNT) {
            debounceCount = 0;
            s_RFCtrlInfo.isWaitReturn = true;
        }
    } else {
        debounceCount = 0;
    }

    if(s_RFCtrlInfo.isWaitReturn) {
        App_RadioFreq_ChangeState(E_RF_RUN_STOP);
    }
}

void App_RadioFreq_Process(void)
{
    static Drv_Timer_t CurrentMonitorTimer;
    static Drv_Timer_t TempMonitorTimer;
    
    // Process the radio frequency module
    App_RadioFreq_UpdateStatus();
    App_RadioFreq_RxDataHandle();
    App_RadioFreq_Monitor();
    App_RadioFreq_CheckProbe();
    // Handle the radio frequency state
    switch(s_RFCtrlInfo.runState)
    {
        case E_RF_RUN_INIT:
            // ?????????
            if(App_Memory_LoadRFParams(&s_RFCtrlInfo.TreatParams)) {
                s_RFCtrlInfo.TempLimit = s_RFCtrlInfo.TreatParams.TempLimit;
                s_RFCtrlInfo.TreatCounts = s_RFCtrlInfo.TreatParams.TreatRemainTimes;
                s_RFCtrlInfo.CurrentHigh = s_RFCtrlInfo.TreatParams.CurrentHigh;
                s_RFCtrlInfo.CurrentLow = s_RFCtrlInfo.TreatParams.CurrentLow;
                
                // ??????
                RF_TransData_t *pTransData = App_Comm_GetRFTransData();
                pTransData->RxConfig.temp_limit = s_RFCtrlInfo.TempLimit;
                
                LOG_I("RF: Parameters loaded - temp_limit=%d, remain_times=%d, current_range=[%d, %d]",
                      s_RFCtrlInfo.TempLimit, s_RFCtrlInfo.TreatCounts,
                      s_RFCtrlInfo.CurrentLow, s_RFCtrlInfo.CurrentHigh);
            } else {
                LOG_E("RF: Failed to load parameters");
                s_RFCtrlInfo.ErrorCode = E_RF_ERROR_READ_PARAMS_FAILED;
            }
            App_RadioFreq_ChangeState(E_RF_RUN_IDLE);
            break;
            
        case E_RF_RUN_IDLE:
            // ??App_RadioFreq_StartCheck??????????
            if(App_RadioFreq_StartCheck()) {
                // ??????????????
                App_RadioFreq_SetWorkParams();

                // pwr_control2????????????????
                Drv_IODevice_ChangeChannel(CHANNEL_RF);
                App_RadioFreq_ChangeState(E_RF_RUN_WORKING);
            }
            break;
            
        case E_RF_RUN_WORKING:
            
            // ???????
            if(App_RadioFreq_StartCheck() == false || 
               s_RFCtrlInfo.TreatRemainTimes == 0){
                App_RadioFreq_ChangeState(E_RF_RUN_STOP);
            } else {
                // ??????10ms????
                if(Drv_Timer_Tick(&CurrentMonitorTimer, RF_CURRENT_MONITOR_PERIOD_MS)) {
                    if(App_RadioFreq_IsCurrentNormal() == false) {
                        // ??????????????????
                    }
                }
                
                // ??????1s????
                if(Drv_Timer_Tick(&TempMonitorTimer, RF_TEMP_MONITOR_PERIOD_MS)) {
                    if(App_RadioFreq_IsHeadTempNormal() == false) {
                        // ??????????
                        App_RadioFreq_ChangeState(E_RF_RUN_STOP);
                    }
                }
                
                // ????
                if(s_RFCtrlInfo.TreatRemainTimes >= TREAT_TASK_TIME) {
                    s_RFCtrlInfo.TreatRemainTimes -= TREAT_TASK_TIME;
                } else {
                    s_RFCtrlInfo.TreatRemainTimes = 0;
                }
            }
            break;
            
        case E_RF_RUN_STOP:
            // ??????
            Drv_IODevice_ChangeChannel(CHANNEL_CLOSE);
            // ????DAC??
            Drv_DAC_SetVoltage(0);
            // CTR_HEAT_HP?????????
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 0);
            if(s_RFCtrlInfo.isWaitReturn) {
                App_RadioFreq_ChangeState(E_RF_RUN_WAIT_RETURN);
                LOG_I("RF: Wait return");
                s_RFCtrlInfo.isWaitReturn = false;
            }
            break;

        case E_RF_RUN_WAIT_RETURN:
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
    // ???????????
    memset(&s_RFCtrlInfo, 0, sizeof(RF_CtrlInfo_t));
    
    // ?????????
    s_RFCtrlInfo.runState = E_RF_RUN_INIT;
    s_RFCtrlInfo.ErrorCode = E_RF_ERROR_NONE;
    s_RFCtrlInfo.WorkLevel = 0;
    s_RFCtrlInfo.TreatRemainTimes = 0;
    s_RFCtrlInfo.TreatCounts = 0;
    s_RFCtrlInfo.Voltage = RF_VOLTAGE_INIT_MV;
    
    // ?????DAC
    Drv_DAC_Init();
    
    // ?????SI5351
    Drv_SI5351_Init();
    
    LOG_I("Radio Frequency module initialized");
}

RF_GetStatus_Reply_t *App_RadioFreq_GetStatus(void)
{
    return &s_RFCtrlInfo.Trans.TxStatus;
}

RF_RunState_EnumDef App_RadioFreq_GetRunState(void)
{
    return s_RFCtrlInfo.runState;
}

/**************************End of file********************************/
