/***********************************************************************************
* @file     : app_comm.c
* @brief    : 
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "app_comm.h"

App_Comm_Info_t s_AppCommInfo;

/* =============================================================================
 * Private Functions
 * ============================================================================= */


void App_Comm_RecvDataHandle(uint8_t *Data)
{
    if(Data == NULL){
        return;
    }

    switch(Data[3])
    {
        case PROTOCOL_MODULE_ULTRASOUND:
            switch(Data[4]) 
            {
                case PROTOCOL_CMD_GET_STATUS:
                    s_AppCommInfo.US.flag.bits.Rely_Status = 1;
                    break;
                case PROTOCOL_CMD_SET_WORK_STATE:
                    s_AppCommInfo.US.RxWorkState.work_state = Data[6];
                    s_AppCommInfo.US.RxWorkState.work_time = Data[7]  | Data[8] << 8;
                    s_AppCommInfo.US.RxWorkState.work_level = Data[9];
                    s_AppCommInfo.US.RxValidFlag[PROTOCOL_CMD_SET_WORK_STATE] = true;
                    break; 
                case PROTOCOL_CMD_SET_CONFIG:
                    s_AppCommInfo.US.RxConfig.frequency = Data[6] | Data[7] << 8;
                    s_AppCommInfo.US.RxConfig.voltage = Data[8] | Data[9] << 8;
                    s_AppCommInfo.US.RxConfig.temp_limit = Data[10] | Data[11] << 8;
                    s_AppCommInfo.US.flag.bits.Rely_Config = 1;
                    s_AppCommInfo.US.RxValidFlag[PROTOCOL_CMD_SET_CONFIG] = true;
                    break;
            }
            break;
        case PROTOCOL_MODULE_RADIO_FREQ:
            switch(Data[4]) 
            {
                case PROTOCOL_CMD_GET_STATUS:
                    s_AppCommInfo.RF.flag.bits.Rely_Status = 1;
                    break;
                case PROTOCOL_CMD_SET_WORK_STATE:
                    s_AppCommInfo.RF.RxWorkState.work_state = Data[6];
                    s_AppCommInfo.RF.RxWorkState.work_time = Data[7] | Data[8] << 8;
                    s_AppCommInfo.RF.RxWorkState.work_level = Data[9];
                    break; 
                case PROTOCOL_CMD_SET_CONFIG:
                    s_AppCommInfo.RF.RxConfig.temp_limit = Data[6] | Data[7] << 8;
                    s_AppCommInfo.RF.flag.bits.Rely_Config = 1;
                    break;
            }
            break;
        case PROTOCOL_MODULE_SHOCKWAVE:
            switch(Data[4]) 
            {
                case PROTOCOL_CMD_GET_STATUS:
                    s_AppCommInfo.SW.flag.bits.Rely_Status = 1;
                    break;
                case PROTOCOL_CMD_SET_WORK_STATE:
                    s_AppCommInfo.SW.RxWorkState.work_state = Data[6];
                    s_AppCommInfo.SW.RxWorkState.work_time = Data[7] | Data[8] << 8;
                    s_AppCommInfo.SW.RxWorkState.work_level = Data[9];
                    s_AppCommInfo.SW.RxWorkState.frequency = Data[10];
                    break; 
            }
            break;
        case PROTOCOL_MODULE_HEAT:
            switch(Data[4]) 
            {
                case PROTOCOL_CMD_GET_STATUS:
                    s_AppCommInfo.Heat.flag.bits.Rely_Status = 1;
                    break;
                case PROTOCOL_CMD_SET_WORK_STATE:
                    s_AppCommInfo.Heat.RxWorkState.work_state = Data[6];
                    s_AppCommInfo.Heat.RxWorkState.work_time = Data[7] | Data[8] << 8;
                    s_AppCommInfo.Heat.RxWorkState.pressure = Data[9];
                    s_AppCommInfo.Heat.RxWorkState.suck_time = Data[10] | Data[11] << 8;
                    s_AppCommInfo.Heat.RxWorkState.release_time = Data[12] | Data[13] << 8;
                    s_AppCommInfo.Heat.RxWorkState.temp_limit = Data[14] | Data[15] << 8;
                    break; 
                case PROTOCOL_CMD_SET_CONFIG:
                    s_AppCommInfo.Heat.RxPreheat.preheat_state = Data[6];
                    s_AppCommInfo.Heat.RxPreheat.work_time = Data[7] | Data[8] << 8;
                    s_AppCommInfo.Heat.RxPreheat.temp_limit = Data[9] | Data[10] << 8;
                    s_AppCommInfo.Heat.flag.bits.Rely_Config = 1;
                    break;
            }
            break;
        default:
            break;
    }
}


UltraSound_TransData_t *App_Comm_GetUSTransData(void)
{
    return &s_AppCommInfo.US;
}

RF_TransData_t *App_Comm_GetRFTransData(void)
{
    return &s_AppCommInfo.RF;
}

SW_TransData_t *App_Comm_GetSWTransData(void)
{
    return &s_AppCommInfo.SW;
}

Heat_TransData_t *App_Comm_GetHeatTransData(void)
{
    return &s_AppCommInfo.Heat;
}


/* =============================================================================
 * Public Functions
 * ============================================================================= */

void App_Comm_Init(void)
{
    memset(&s_AppCommInfo, 0, sizeof(App_Comm_Info_t));
}

void App_Comm_Process(void)
{
    // Process the communication module
    // This function can be called periodically to process received data

}

/**************************End of file********************************/
