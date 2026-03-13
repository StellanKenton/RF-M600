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
 * @brief Calculate voltage from work level (level 1-20 maps to 11-30V)
 * @param level Work level (0-20)
 * @retval Voltage in mV
 */
static uint16_t App_RadioFreq_CalculateVoltage(uint8_t level)
{
    uint16_t voltage = 0;
    if(level == 0) {
        return RF_VOLTAGE_INIT_MV;
    }
    if(level > RF_WORK_LEVEL_MAX) {
        level = RF_WORK_LEVEL_MAX;
    }
    voltage = RF_VOLTAGE_MIN_MV + ((level - 1) * (RF_VOLTAGE_MAX_MV - RF_VOLTAGE_MIN_MV)) / (RF_WORK_LEVEL_MAX - 1);
    if(voltage > RF_VOLTAGE_MAX_MV) {
        voltage = RF_VOLTAGE_MAX_MV;
    } else if(voltage < RF_VOLTAGE_MIN_MV) {
        voltage = RF_VOLTAGE_MIN_MV;
    }
    return voltage;
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
    s_RFCtrlInfo.Trans.TxStatus.remain_time = s_RFCtrlInfo.TreatCounts / 1000;  /* ms -> s */
    s_RFCtrlInfo.Trans.TxStatus.work_level = s_RFCtrlInfo.WorkLevel;
    s_RFCtrlInfo.Trans.TxStatus.head_temp = s_RFCtrlInfo.HeadTemp;

    /* Get connection state from treat mgr (probe + foot switch) */
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
    if(pTransData == NULL)
    {
        return;
    }

    /* RF work-state packets are parsed in app_comm and must be mirrored here
       before the state machine can react to start/stop/reset commands. */
    s_RFCtrlInfo.Trans.RxWorkState = pTransData->RxWorkState;
    s_RFCtrlInfo.Trans.RxConfig = pTransData->RxConfig;
}

void App_RadioFreq_WorkTimeHandle(void)
{
    static uint8_t s_lastRxWorkState = 0x00;

    if(s_RFCtrlInfo.Trans.RxWorkState.work_state != s_lastRxWorkState) {
        if (s_RFCtrlInfo.Trans.RxWorkState.work_state == WORK_STATE_RESET && s_lastRxWorkState != WORK_STATE_RESET) {
            s_RFCtrlInfo.TreatCountsState = E_TREAT_TIMES_RESET;
        }
        s_lastRxWorkState = s_RFCtrlInfo.Trans.RxWorkState.work_state;
    }

    switch(s_RFCtrlInfo.TreatCountsState)
    {
        case E_TREAT_TIMES_POWER_ON:
            if(s_RFCtrlInfo.Trans.RxWorkState.work_time > 0 && s_RFCtrlInfo.TreatRemainTimes > 0)
            {
                s_RFCtrlInfo.TreatCounts = s_RFCtrlInfo.Trans.RxWorkState.work_time * 1000;  /* s -> ms */
                s_RFCtrlInfo.WorkLevel = s_RFCtrlInfo.Trans.RxWorkState.work_level;
                s_RFCtrlInfo.TreatCountsState = E_TREAT_TIMES_WORKING;
                s_RFCtrlInfo.TreatParams.TreatRemainTimes = s_RFCtrlInfo.TreatRemainTimes - 1;
                s_RFCtrlInfo.TreatRemainTimes--;
                App_Memory_SaveRFParams(&s_RFCtrlInfo.TreatParams);
                LOG_I("RF: Remaining treat times decreased to: %d", s_RFCtrlInfo.TreatRemainTimes);
            }
            break;
        case E_TREAT_TIMES_RESET:
            if(s_RFCtrlInfo.Trans.RxWorkState.work_time > 0 && s_RFCtrlInfo.TreatRemainTimes > 0)
            {
                s_RFCtrlInfo.TreatCounts = s_RFCtrlInfo.Trans.RxWorkState.work_time * 1000;  /* s -> ms */
                s_RFCtrlInfo.WorkLevel = s_RFCtrlInfo.Trans.RxWorkState.work_level;
                s_RFCtrlInfo.TreatCountsState = E_TREAT_TIMES_WORKING;
                s_RFCtrlInfo.TreatParams.TreatRemainTimes = s_RFCtrlInfo.TreatRemainTimes - 1;
                s_RFCtrlInfo.TreatRemainTimes--;
                App_Memory_SaveRFParams(&s_RFCtrlInfo.TreatParams);
                LOG_I("RF: Remaining treat times decreased to: %d", s_RFCtrlInfo.TreatRemainTimes);
            }
            break;
        case E_TREAT_TIMES_WORKING:
            if(s_RFCtrlInfo.TreatCounts > 0 && s_RFCtrlInfo.runState == E_RF_RUN_WORKING)
            {
                if(s_RFCtrlInfo.TreatCounts >= TREAT_TASK_TIME) {
                    s_RFCtrlInfo.TreatCounts -= TREAT_TASK_TIME;
                } else {
                    s_RFCtrlInfo.TreatCounts = 0;
                }
            }
            if(s_RFCtrlInfo.TreatCounts == 0)
            {
                s_RFCtrlInfo.TreatCountsState = E_TREAT_TIMES_WAIT;
            }
            break;
        case E_TREAT_TIMES_WAIT:
            break;
        default:
            break;
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
                Drv_IODevice_StartBuzzer(2000);
                break;
            case E_RF_RUN_STOP:
                LOG_I("RF state changed to STOP");
                Drv_IODevice_StartBuzzer(2000);
                break;
            default:
                break;
        }
    }
}

void App_RadioFreq_Monitor(void)
{
}

bool App_RadioFreq_StartCheck()
{
    if(s_RFCtrlInfo.Trans.RxWorkState.work_state != WORK_STATE_START) {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_INVALID_PARAMS;
        s_RFCtrlInfo.LastStartState = 0x00;
        return false;
    }

    if(s_RFCtrlInfo.Trans.RxWorkState.work_time == 0 || s_RFCtrlInfo.Trans.RxWorkState.work_time > 3600) {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_INVALID_PARAMS;
        s_RFCtrlInfo.LastStartState = 0x01;
        return false;
    }

    if(s_RFCtrlInfo.Trans.RxWorkState.work_level == 0 || s_RFCtrlInfo.Trans.RxWorkState.work_level > RF_WORK_LEVEL_MAX) {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_INVALID_PARAMS;
        s_RFCtrlInfo.LastStartState = 0x02;
        return false;
    }

    if(App_TreatMgr_GetFootSwitchClosed()) {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_INVALID_PARAMS;
        s_RFCtrlInfo.LastStartState = 0x03;
        return false;
    }

    if(App_TreatMgr_GetProbeStatus() != E_IODEVICE_MODE_RADIO_FREQUENCY) {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_PROBE_NOT_CONNECTED;
        s_RFCtrlInfo.LastStartState = 0x04;
        return false;
    }

    if(s_RFCtrlInfo.TreatCounts == 0) {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_INVALID_PARAMS;
        s_RFCtrlInfo.LastStartState = 0x05;
        return false;
    }

    if(s_RFCtrlInfo.TreatRemainTimes == 0) {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_INVALID_PARAMS;
        s_RFCtrlInfo.LastStartState = 0x06;
        return false;
    }

    return true;
}

void App_RadioFreq_SetWorkParams(void)
{
    s_RFCtrlInfo.WorkLevel = s_RFCtrlInfo.Trans.RxWorkState.work_level;
    s_RFCtrlInfo.VoltageTarget = App_RadioFreq_CalculateVoltage(s_RFCtrlInfo.WorkLevel);
    s_RFCtrlInfo.Voltage = RF_VOLTAGE_INIT_MV;
    Drv_DAC_SetVoltage(s_RFCtrlInfo.Voltage);
    Drv_Delay_ms(100);
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
        if(currentVoltage != RF_VOLTAGE_INIT_MV)
        {
            newVoltage = RF_VOLTAGE_INIT_MV;
            Drv_DAC_SetVoltage(newVoltage);
            s_RFCtrlInfo.Voltage = newVoltage;
            LOG_I("RF: Current too low (%d mV), voltage set to 7V", current);
        }
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_CURRENT_TOO_LOW;
    }
    else if(current >= s_RFCtrlInfo.CurrentLow)
    {
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
        if(currentVoltage != RF_VOLTAGE_INIT_MV)
        {
            newVoltage = RF_VOLTAGE_INIT_MV;
            Drv_DAC_SetVoltage(newVoltage);
            s_RFCtrlInfo.Voltage = newVoltage;
            LOG_I("RF: Current below range (%d mV), voltage set to 7V", current);
        }
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_CURRENT_TOO_LOW;
    }

    return isNormal;
}


uint16_t App_RadioFreq_GetProbeTemp(void)
{
    return 350;
}

bool App_RadioFreq_IsHeadTempNormal(void)
{
    bool isNormal = true;
    uint16_t temp = App_RadioFreq_GetProbeTemp();
    s_RFCtrlInfo.HeadTemp = temp;

    if(s_RFCtrlInfo.HeadTemp > s_RFCtrlInfo.TempLimit)
    {
        s_RFCtrlInfo.ErrorCode = E_RF_ERROR_TEMP_TOO_HIGH;
        LOG_W("RF: Head temperature too high: %d (limit: %d)",
              s_RFCtrlInfo.HeadTemp, s_RFCtrlInfo.TempLimit);
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
    App_RadioFreq_UpdateStatus();
    App_RadioFreq_RxDataHandle();
    App_RadioFreq_WorkTimeHandle();
    App_RadioFreq_CheckProbe();

    switch(s_RFCtrlInfo.runState)
    {
        case E_RF_RUN_INIT:
            s_RFCtrlInfo.TreatCountsState = E_TREAT_TIMES_POWER_ON;
            {
                const RF_TreatParams_t *pParams = App_Memory_GetRFParams();
                s_RFCtrlInfo.TreatParams = *pParams;
                s_RFCtrlInfo.TempLimit = pParams->TempLimit;
                s_RFCtrlInfo.TreatRemainTimes = pParams->TreatRemainTimes;
                s_RFCtrlInfo.CurrentHigh = pParams->CurrentHigh;
                s_RFCtrlInfo.CurrentLow = pParams->CurrentLow;
                s_RFCtrlInfo.Trans.RxConfig.temp_limit = pParams->TempLimit;
                LOG_I("RF: Parameters loaded - temp_limit=%d, remain_times=%d, current_range=[%d, %d]",
                      s_RFCtrlInfo.TempLimit, s_RFCtrlInfo.TreatRemainTimes,
                      s_RFCtrlInfo.CurrentLow, s_RFCtrlInfo.CurrentHigh);
            }
            s_RFCtrlInfo.Voltage = 700;     // 7.00V
            s_RFCtrlInfo.VoltageTarget = 700;
            // 1.00MHz
            Drv_SI5351_SetFrequency(1000);
            Drv_IODevice_ChangeChannel(CHANNEL_RF);
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 1);
            App_RadioFreq_ChangeState(E_RF_RUN_IDLE);
            break;

        case E_RF_RUN_IDLE:
            if(App_RadioFreq_StartCheck()) {
                App_RadioFreq_SetWorkParams();
                Drv_SI5351_SetComplementaryPWM(true);
                Drv_IODevice_ChangeChannel(CHANNEL_READY);
                App_RadioFreq_ChangeState(E_RF_RUN_WORKING);
            }
            break;

        case E_RF_RUN_WORKING:
            if(App_RadioFreq_StartCheck() == false ||
               App_RadioFreq_IsCurrentNormal() == false ||
               App_RadioFreq_IsHeadTempNormal() == false) {
                App_RadioFreq_ChangeState(E_RF_RUN_STOP);
            }
            break;

        case E_RF_RUN_STOP:
            Drv_SI5351_SetComplementaryPWM(false);
            Drv_IODevice_ChangeChannel(CHANNEL_CLOSE);
            Drv_DAC_SetVoltage(0);
            App_RadioFreq_ChangeState(E_RF_RUN_IDLE);
            if(s_RFCtrlInfo.isWaitReturn) {
                Drv_IODevice_WritePin(E_GPIO_OUT_CTR_HEAT_HP, 0);
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
    memset(&s_RFCtrlInfo, 0, sizeof(RF_CtrlInfo_t));

    s_RFCtrlInfo.runState = E_RF_RUN_INIT;
    s_RFCtrlInfo.ErrorCode = E_RF_ERROR_NONE;
    s_RFCtrlInfo.WorkLevel = 0;
    s_RFCtrlInfo.TreatRemainTimes = 0;
    s_RFCtrlInfo.TreatCounts = 0;
    s_RFCtrlInfo.Voltage = RF_VOLTAGE_INIT_MV;

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
