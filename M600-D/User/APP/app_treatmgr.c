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
#include "drv_si5351.h"
#include "drv_dac.h"

TreatMgr_t s_TreatMgr;

/* Board temperature monitoring parameters */
#define BOARD_TEMP_FAN_ON_THRESHOLD     850     ///< Fan start temperature threshold (85C = 850 * 0.1C)
#define BOARD_TEMP_FAN_OFF_THRESHOLD    800     ///< Fan stop temperature threshold (80C = 800 * 0.1C), hysteresis to avoid frequent switching
#define BOARD_TEMP_MONITOR_PERIOD_MS    1000    ///< Board temperature monitoring period (1s)

/**
 * @brief Read board temperature from Heat_REF01 and Heat_REF02
 * @retval Board temperature in 0.1°C, returns average of two sensors
 * @note Calibration required based on actual temperature sensor characteristics
 *       For NTC thermistor: use Steinhart-Hart equation or lookup table
 *       For linear sensor: use linear conversion formula
 */
static uint16_t App_TreatMgr_ReadBoardTemp(void)
{
    uint16_t temp1_voltage = Drv_ADC_ReadVoltage(E_ADC_CHANNEL_Heat_REF01);
    uint16_t temp2_voltage = Drv_ADC_ReadVoltage(E_ADC_CHANNEL_Heat_REF02);
    
    // TODO: Convert voltage to temperature based on actual sensor characteristics
    // Using simplified linear conversion as placeholder
    // Calibration required per sensor datasheet:
    // - NTC thermistor: use Steinhart-Hart equation or lookup table
    // - Linear sensor: use linear conversion formula
    // - Other sensors: convert based on characteristic curve
    
    // Average of two sensors (if both valid)
    uint16_t avg_voltage = (temp1_voltage + temp2_voltage) / 2;
    
    // Simplified linear conversion (calibrate for actual hardware)
    // Assumption: 0V = 0C, 3.3V = 100C = 1000 * 0.1C
    // temp = (voltage_mv * 1000) / 3300
    // Note: placeholder implementation, replace with actual sensor characteristics
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
        // Temperature above 85C, start fan
        if(!fanState)
        {
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_FAN, 1);
            fanState = true;
            LOG_I("Board temperature too high (%d * 0.1°C), fan started", boardTemp);
        }
    }
    else if(boardTemp < BOARD_TEMP_FAN_OFF_THRESHOLD)
    {
        // Temperature below 80C, stop fan (hysteresis to avoid frequent switching)
        if(fanState)
        {
            Drv_IODevice_WritePin(E_GPIO_OUT_CTR_FAN, 0);
            fanState = false;
            LOG_I("Board temperature normal (%d * 0.1°C), fan stopped", boardTemp);
        }
    }
    // Between 80-85C keep current state
}

void App_TreatMgr_Init(void)
{
    // Initialize the treatment manager module
    s_TreatMgr.eState = E_TREATMGR_STATE_IDLE;
    Log_RegisterFunction("setprobe", Drv_IODevice_SetProbeStatus);
    s_TreatMgr.eProbeStatus = E_IODEVICE_MODE_NOT_CONNECTED;
    s_TreatMgr.eFootSwitchClosed = false;
    // Initialize DAC
    Drv_DAC_Init();
    
    // Initialize SI5351
    Drv_SI5351_Init();
}

IODevice_WorkingMode_EnumDef App_TreatMgr_GetProbeStatus(void)
{
    return s_TreatMgr.eProbeStatus;
}

bool App_TreatMgr_GetFootSwitchClosed(void)
{
    return s_TreatMgr.eFootSwitchClosed;
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
			case E_TREATMGR_STATE_MAX:
				break;
        }
    }
}



void ProbeStatusCheck()
{
    static IODevice_WorkingMode_EnumDef pendingStatus = E_IODEVICE_MODE_NOT_CONNECTED;
    static uint16_t debounceCount = 0;

    IODevice_WorkingMode_EnumDef curStatus = Drv_IODevice_GetProbeStatus();

    if(curStatus == s_TreatMgr.eProbeStatus) {
        debounceCount = 0;
        return;
    }

    if(curStatus == pendingStatus) {
        debounceCount++;
        if(debounceCount >= PROBE_STATUS_DEBOUNCE_CNT) {
            s_TreatMgr.preProbeStaus = s_TreatMgr.eProbeStatus;
            s_TreatMgr.eProbeStatus = curStatus;
            debounceCount = 0;
            pendingStatus = curStatus;
            switch(s_TreatMgr.eProbeStatus)
            {
            case E_IODEVICE_MODE_ULTRASOUND:
                LOG_I("Probe***** status changed to ULTRASOUND");
                break;
            case E_IODEVICE_MODE_SHOCKWAVE:
                LOG_I("Probe***** status changed to SHOCKWAVE");
                break;
            case E_IODEVICE_MODE_RADIO_FREQUENCY:
                LOG_I("Probe***** status changed to RADIO_FREQUENCY");
                break;
            case E_IODEVICE_MODE_NEGATIVE_PRESSURE_HEAT:
                LOG_I("Probe***** status changed to NEGATIVE_PRESSURE_HEAT");
                break;
            case E_IODEVICE_MODE_NOT_CONNECTED:
                LOG_I("Probe***** status changed to NOT_CONNECTED");
                break;  
            case E_IODEVICE_MODE_ERROR:
                LOG_I("Probe status changed to ERROR");
                break;
            default:
                LOG_I("Probe status changed to UNKNOWN");
                break;
            }
        }
    } else {
        pendingStatus = curStatus;
        debounceCount = 1;
    }
}

void App_TreatMgr_CheckWaitReturn(void)
{
    if(App_Shockwave_GetRunState() == E_SW_RUN_WAIT_RETURN) {
        App_TreatMgr_ChangeState(E_TREATMGR_STATE_IDLE);
        App_Shockwave_ChangeState(E_SW_RUN_INIT);
    }

    if(App_RadioFreq_GetRunState() == E_RF_RUN_WAIT_RETURN) {
        App_TreatMgr_ChangeState(E_TREATMGR_STATE_IDLE);
        App_RadioFreq_ChangeState(E_RF_RUN_INIT);
    }

    if(App_Ultrasound_GetRunState() == E_US_RUN_WAIT_RETURN) {
        App_TreatMgr_ChangeState(E_TREATMGR_STATE_IDLE);
        App_Ultrasound_ChangeState(E_US_RUN_INIT);
    }

    if(App_NegPrsHeat_GetRunState() == E_NPH_RUN_WAIT_RETURN) {
        App_TreatMgr_ChangeState(E_TREATMGR_STATE_IDLE);
        App_NegPrsHeat_ChangeState(E_NPH_RUN_INIT);
    }
}

void App_TreatMgr_Process(void)
{
    static Drv_Timer_t TreatMgrTimer;
    static Drv_Timer_t BoardTempMonitorTimer;

    
    if(Drv_Timer_Tick(&TreatMgrTimer, TREAT_TASK_TIME) == false){
        return;
    }

    // Process buzzer control (every loop for timely response)
    Drv_IODevice_ProcessBuzzer();
    // Process treatment manager: refresh probe and foot switch status
    ProbeStatusCheck();
    s_TreatMgr.eFootSwitchClosed = Drv_IODevice_GetFootSwitchState();

    // Board temperature monitoring and fan control (1s period)
    if(Drv_Timer_Tick(&BoardTempMonitorTimer, BOARD_TEMP_MONITOR_PERIOD_MS)){
        App_TreatMgr_ControlFan();
    }
    // Check if wait for reconnection is needed
    App_TreatMgr_CheckWaitReturn();

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
