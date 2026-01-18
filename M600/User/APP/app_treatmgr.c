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
#include "log.h"

TreatMgr_t s_TreatMgr;


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

void App_TreatMgr_ChangeCheck(IODevice_WorkingMode_EnumDef curProbe) 
{
    if(curProbe != s_TreatMgr.preProbeStaus){
        
    }
}

void App_TreatMgr_Process(void)
{
    // Process the treatment manager module
    ProbeStatusCheck();
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
            App_TreatMgr_ChangeCheck(E_IODEVICE_MODE_RADIO_FREQUENCY);
            break;
        case E_TREATMGR_STATE_SHOCK_WAVE:
            // Handle shock wave state
            App_TreatMgr_ChangeCheck(E_IODEVICE_MODE_SHOCKWAVE);
            break;
        case E_TREATMGR_STATE_NEGATIVE_PRESSURE_HEAT:
            // Handle negative pressure heat state
            App_TreatMgr_ChangeCheck(E_IODEVICE_MODE_NEGATIVE_PRESSURE_HEAT);
            break;
        case E_TREATMGR_STATE_ULTRASOUND:
            // Handle ultrasound state
            App_TreatMgr_ChangeCheck(E_IODEVICE_MODE_ULTRASOUND);
            break;
        case E_TREATMGR_STATE_ERROR:
            // Handle error state
            break;
    }
}

/**************************End of file********************************/
