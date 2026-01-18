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

/**
 * @brief Build protocol frame common part (header, direction, module, cmd, data_len, tail)
 * @param pTxData Pointer to transmit data buffer
 * @param direction Transmission direction
 * @param module Module command
 * @param cmd Command code
 * @param data_len Data field length
 * @return Total frame length
 */
static uint8_t App_Comm_BuildFrameCommon(uint8_t *pTxData, uint8_t direction, 
                                         uint8_t module, uint8_t cmd, uint8_t data_len)
{
    uint8_t idx = 0;
    
    if (pTxData == NULL)
    {
        return 0;
    }
    
    // Header
    pTxData[idx++] = PROTOCOL_HEADER_0;
    pTxData[idx++] = PROTOCOL_HEADER_1;
    
    // Direction
    pTxData[idx++] = direction;
    
    // Module
    pTxData[idx++] = module;
    
    // Command
    pTxData[idx++] = cmd;
    
    // Data length
    pTxData[idx++] = data_len;
    
    // Data field will be filled by caller
    // Tail will be filled after data field
    
    return idx; // Return index where data field starts
}

/**
 * @brief Add frame tail
 * @param pTxData Pointer to transmit data buffer
 * @param idx Current index in buffer
 */
static void App_Comm_AddFrameTail(uint8_t *pTxData, uint8_t idx)
{
    if (pTxData == NULL)
    {
        return;
    }
    
    pTxData[idx++] = PROTOCOL_TAIL_0;
    pTxData[idx++] = PROTOCOL_TAIL_1;
}

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
            break;
        case PROTOCOL_MODULE_SHOCKWAVE:
            break;
        case PROTOCOL_MODULE_HEAT:
            break;
        default:
            break;
    }
}


UltraSound_TransData_t *App_Comm_GetUSTransData(void)
{
    return &s_AppCommInfo.US;
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
