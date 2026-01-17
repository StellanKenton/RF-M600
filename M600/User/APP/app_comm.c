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

/**
 * @brief Write uint16_t value in little-endian format
 * @param pBuf Pointer to buffer
 * @param idx Current index
 * @param value Value to write
 * @return New index after writing
 */
static uint8_t App_Comm_WriteU16(uint8_t *pBuf, uint8_t idx, uint16_t value)
{
    if (pBuf == NULL)
    {
        return idx;
    }
    
    pBuf[idx++] = (uint8_t)(value & 0xFF);
    pBuf[idx++] = (uint8_t)((value >> 8) & 0xFF);
    return idx;
}

/**
 * @brief Read uint16_t value in little-endian format
 * @param pBuf Pointer to buffer
 * @param idx Current index
 * @param pValue Pointer to store value
 * @return New index after reading
 */
static uint8_t App_Comm_ReadU16(const uint8_t *pBuf, uint8_t idx, uint16_t *pValue)
{
    if (pBuf == NULL || pValue == NULL)
    {
        return idx;
    }
    
    *pValue = pBuf[idx] | (pBuf[idx + 1] << 8);
    return idx + 2;
}

/* =============================================================================
 * Public Functions - Send Pack Functions
 * ============================================================================= */

/**
 * @brief Build US Get Status send packet
 * @param pData Pointer to send data structure
 * @param pTxData Pointer to transmit buffer
 * @param pLen Pointer to store packet length
 * @return 0: Success, -1: Error
 */
int8_t App_Comm_BuildUS_GetStatus_Send(const US_GetStatus_Send_t *pData, 
                                        uint8_t *pTxData, uint8_t *pLen)
{
    uint8_t idx;
    
    if (pData == NULL || pTxData == NULL || pLen == NULL)
    {
        return -1;
    }
    
    // Build common frame
    idx = App_Comm_BuildFrameCommon(pTxData, PROTOCOL_DIR_HOST_TO_DEV, 
                                    PROTOCOL_MODULE_ULTRASOUND, 
                                    PROTOCOL_CMD_GET_STATUS, 1);
    
    // Data field
    pTxData[idx++] = pData->dummy;
    
    // Add tail
    App_Comm_AddFrameTail(pTxData, idx);
    
    *pLen = idx + 2; // Total length
    return 0;
}

/**
 * @brief Build US Set Work State send packet
 * @param pData Pointer to send data structure
 * @param pTxData Pointer to transmit buffer
 * @param pLen Pointer to store packet length
 * @return 0: Success, -1: Error
 */
int8_t App_Comm_BuildUS_SetWorkState_Send(const US_SetWorkState_Send_t *pData, 
                                          uint8_t *pTxData, uint8_t *pLen)
{
    uint8_t idx;
    
    if (pData == NULL || pTxData == NULL || pLen == NULL)
    {
        return -1;
    }
    
    // Build common frame
    idx = App_Comm_BuildFrameCommon(pTxData, PROTOCOL_DIR_HOST_TO_DEV, 
                                    PROTOCOL_MODULE_ULTRASOUND, 
                                    PROTOCOL_CMD_SET_WORK_STATE, 4);
    
    // Data field
    pTxData[idx++] = pData->work_state;
    idx = App_Comm_WriteU16(pTxData, idx, pData->work_time);
    pTxData[idx++] = pData->work_level;
    
    // Add tail
    App_Comm_AddFrameTail(pTxData, idx);
    
    *pLen = idx + 2; // Total length
    return 0;
}

/**
 * @brief Build US Set Config send packet
 * @param pData Pointer to send data structure
 * @param pTxData Pointer to transmit buffer
 * @param pLen Pointer to store packet length
 * @return 0: Success, -1: Error
 */
int8_t App_Comm_BuildUS_SetConfig_Send(const US_SetConfig_Send_t *pData, 
                                        uint8_t *pTxData, uint8_t *pLen)
{
    uint8_t idx;
    
    if (pData == NULL || pTxData == NULL || pLen == NULL)
    {
        return -1;
    }
    
    // Build common frame
    idx = App_Comm_BuildFrameCommon(pTxData, PROTOCOL_DIR_HOST_TO_DEV, 
                                    PROTOCOL_MODULE_ULTRASOUND, 
                                    PROTOCOL_CMD_SET_CONFIG, 6);
    
    // Data field
    idx = App_Comm_WriteU16(pTxData, idx, pData->frequency);
    idx = App_Comm_WriteU16(pTxData, idx, pData->voltage);
    idx = App_Comm_WriteU16(pTxData, idx, pData->temp_limit);
    
    // Add tail
    App_Comm_AddFrameTail(pTxData, idx);
    
    *pLen = idx + 2; // Total length
    return 0;
}

/**
 * @brief Build RF Get Status send packet
 * @param pData Pointer to send data structure
 * @param pTxData Pointer to transmit buffer
 * @param pLen Pointer to store packet length
 * @return 0: Success, -1: Error
 */
int8_t App_Comm_BuildRF_GetStatus_Send(const RF_GetStatus_Send_t *pData, 
                                        uint8_t *pTxData, uint8_t *pLen)
{
    uint8_t idx;
    
    if (pData == NULL || pTxData == NULL || pLen == NULL)
    {
        return -1;
    }
    
    // Build common frame
    idx = App_Comm_BuildFrameCommon(pTxData, PROTOCOL_DIR_HOST_TO_DEV, 
                                    PROTOCOL_MODULE_RADIO_FREQ, 
                                    PROTOCOL_CMD_GET_STATUS, 1);
    
    // Data field
    pTxData[idx++] = pData->dummy;
    
    // Add tail
    App_Comm_AddFrameTail(pTxData, idx);
    
    *pLen = idx + 2; // Total length
    return 0;
}

/**
 * @brief Build RF Set Work State send packet
 * @param pData Pointer to send data structure
 * @param pTxData Pointer to transmit buffer
 * @param pLen Pointer to store packet length
 * @return 0: Success, -1: Error
 */
int8_t App_Comm_BuildRF_SetWorkState_Send(const RF_SetWorkState_Send_t *pData, 
                                          uint8_t *pTxData, uint8_t *pLen)
{
    uint8_t idx;
    
    if (pData == NULL || pTxData == NULL || pLen == NULL)
    {
        return -1;
    }
    
    // Build common frame
    idx = App_Comm_BuildFrameCommon(pTxData, PROTOCOL_DIR_HOST_TO_DEV, 
                                    PROTOCOL_MODULE_RADIO_FREQ, 
                                    PROTOCOL_CMD_SET_WORK_STATE, 4);
    
    // Data field
    pTxData[idx++] = pData->work_state;
    idx = App_Comm_WriteU16(pTxData, idx, pData->work_time);
    pTxData[idx++] = pData->work_level;
    
    // Add tail
    App_Comm_AddFrameTail(pTxData, idx);
    
    *pLen = idx + 2; // Total length
    return 0;
}

/**
 * @brief Build RF Set Config send packet
 * @param pData Pointer to send data structure
 * @param pTxData Pointer to transmit buffer
 * @param pLen Pointer to store packet length
 * @return 0: Success, -1: Error
 */
int8_t App_Comm_BuildRF_SetConfig_Send(const RF_SetConfig_Send_t *pData, 
                                        uint8_t *pTxData, uint8_t *pLen)
{
    uint8_t idx;
    
    if (pData == NULL || pTxData == NULL || pLen == NULL)
    {
        return -1;
    }
    
    // Build common frame
    idx = App_Comm_BuildFrameCommon(pTxData, PROTOCOL_DIR_HOST_TO_DEV, 
                                    PROTOCOL_MODULE_RADIO_FREQ, 
                                    PROTOCOL_CMD_SET_CONFIG, 2);
    
    // Data field
    idx = App_Comm_WriteU16(pTxData, idx, pData->temp_limit);
    
    // Add tail
    App_Comm_AddFrameTail(pTxData, idx);
    
    *pLen = idx + 2; // Total length
    return 0;
}

/**
 * @brief Build SW Get Status send packet
 * @param pData Pointer to send data structure
 * @param pTxData Pointer to transmit buffer
 * @param pLen Pointer to store packet length
 * @return 0: Success, -1: Error
 */
int8_t App_Comm_BuildSW_GetStatus_Send(const SW_GetStatus_Send_t *pData, 
                                        uint8_t *pTxData, uint8_t *pLen)
{
    uint8_t idx;
    
    if (pData == NULL || pTxData == NULL || pLen == NULL)
    {
        return -1;
    }
    
    // Build common frame
    idx = App_Comm_BuildFrameCommon(pTxData, PROTOCOL_DIR_HOST_TO_DEV, 
                                    PROTOCOL_MODULE_SHOCKWAVE, 
                                    PROTOCOL_CMD_GET_STATUS, 1);
    
    // Data field
    pTxData[idx++] = pData->dummy;
    
    // Add tail
    App_Comm_AddFrameTail(pTxData, idx);
    
    *pLen = idx + 2; // Total length
    return 0;
}

/**
 * @brief Build SW Set Work State send packet
 * @param pData Pointer to send data structure
 * @param pTxData Pointer to transmit buffer
 * @param pLen Pointer to store packet length
 * @return 0: Success, -1: Error
 */
int8_t App_Comm_BuildSW_SetWorkState_Send(const SW_SetWorkState_Send_t *pData, 
                                          uint8_t *pTxData, uint8_t *pLen)
{
    uint8_t idx;
    
    if (pData == NULL || pTxData == NULL || pLen == NULL)
    {
        return -1;
    }
    
    // Build common frame
    idx = App_Comm_BuildFrameCommon(pTxData, PROTOCOL_DIR_HOST_TO_DEV, 
                                    PROTOCOL_MODULE_SHOCKWAVE, 
                                    PROTOCOL_CMD_SET_WORK_STATE, 5);
    
    // Data field
    pTxData[idx++] = pData->work_state;
    idx = App_Comm_WriteU16(pTxData, idx, pData->work_time);
    pTxData[idx++] = pData->work_level;
    pTxData[idx++] = pData->frequency;
    
    // Add tail
    App_Comm_AddFrameTail(pTxData, idx);
    
    *pLen = idx + 2; // Total length
    return 0;
}

/**
 * @brief Build Heat Get Status send packet
 * @param pData Pointer to send data structure
 * @param pTxData Pointer to transmit buffer
 * @param pLen Pointer to store packet length
 * @return 0: Success, -1: Error
 */
int8_t App_Comm_BuildHeat_GetStatus_Send(const Heat_GetStatus_Send_t *pData, 
                                          uint8_t *pTxData, uint8_t *pLen)
{
    uint8_t idx;
    
    if (pData == NULL || pTxData == NULL || pLen == NULL)
    {
        return -1;
    }
    
    // Build common frame
    idx = App_Comm_BuildFrameCommon(pTxData, PROTOCOL_DIR_HOST_TO_DEV, 
                                    PROTOCOL_MODULE_HEAT, 
                                    PROTOCOL_CMD_GET_STATUS, 1);
    
    // Data field
    pTxData[idx++] = pData->dummy;
    
    // Add tail
    App_Comm_AddFrameTail(pTxData, idx);
    
    *pLen = idx + 2; // Total length
    return 0;
}

/**
 * @brief Build Heat Set Work State send packet
 * @param pData Pointer to send data structure
 * @param pTxData Pointer to transmit buffer
 * @param pLen Pointer to store packet length
 * @return 0: Success, -1: Error
 */
int8_t App_Comm_BuildHeat_SetWorkState_Send(const Heat_SetWorkState_Send_t *pData, 
                                            uint8_t *pTxData, uint8_t *pLen)
{
    uint8_t idx;
    
    if (pData == NULL || pTxData == NULL || pLen == NULL)
    {
        return -1;
    }
    
    // Build common frame
    idx = App_Comm_BuildFrameCommon(pTxData, PROTOCOL_DIR_HOST_TO_DEV, 
                                    PROTOCOL_MODULE_HEAT, 
                                    PROTOCOL_CMD_SET_WORK_STATE, 8);
    
    // Data field
    pTxData[idx++] = pData->work_state;
    idx = App_Comm_WriteU16(pTxData, idx, pData->work_time);
    pTxData[idx++] = pData->pressure;
    pTxData[idx++] = pData->suck_time;
    pTxData[idx++] = pData->release_time;
    idx = App_Comm_WriteU16(pTxData, idx, pData->temp_limit);
    
    // Add tail
    App_Comm_AddFrameTail(pTxData, idx);
    
    *pLen = idx + 2; // Total length
    return 0;
}

/**
 * @brief Build Heat Set Preheat send packet
 * @param pData Pointer to send data structure
 * @param pTxData Pointer to transmit buffer
 * @param pLen Pointer to store packet length
 * @return 0: Success, -1: Error
 */
int8_t App_Comm_BuildHeat_SetPreheat_Send(const Heat_SetPreheat_Send_t *pData, 
                                          uint8_t *pTxData, uint8_t *pLen)
{
    uint8_t idx;
    
    if (pData == NULL || pTxData == NULL || pLen == NULL)
    {
        return -1;
    }
    
    // Build common frame
    idx = App_Comm_BuildFrameCommon(pTxData, PROTOCOL_DIR_HOST_TO_DEV, 
                                    PROTOCOL_MODULE_HEAT, 
                                    PROTOCOL_CMD_SET_CONFIG, 5);
    
    // Data field
    pTxData[idx++] = pData->preheat_state;
    idx = App_Comm_WriteU16(pTxData, idx, pData->work_time);
    idx = App_Comm_WriteU16(pTxData, idx, pData->temp_limit);
    
    // Add tail
    App_Comm_AddFrameTail(pTxData, idx);
    
    *pLen = idx + 2; // Total length
    return 0;
}

/* =============================================================================
 * Public Functions - Receive Parse Functions
 * ============================================================================= */

/**
 * @brief Parse received data and fill corresponding structure
 * @param pRevData Pointer to received data buffer
 * @param revLen Received data length
 * @return 0: Success, -1: Error
 */
int8_t App_Comm_ParseReceive(const uint8_t *pRevData, uint8_t revLen)
{
    uint8_t module, cmd, data_len;
    uint8_t data_idx;
    
    if (pRevData == NULL || revLen < 7) // Minimum frame length: header(2) + dir(1) + module(1) + cmd(1) + len(1) + tail(2)
    {
        return -1;
    }
    
    // Check header
    if (pRevData[0] != PROTOCOL_HEADER_0 || pRevData[1] != PROTOCOL_HEADER_1)
    {
        return -1;
    }
    
    // Check direction (should be from device to host for received data)
    if (pRevData[2] != PROTOCOL_DIR_DEV_TO_HOST)
    {
        return -1;
    }
    
    // Get module, cmd, data_len
    module = pRevData[3];
    cmd = pRevData[4];
    data_len = pRevData[5];
    data_idx = 6; // Data field starts at index 6
    
    // Check tail
    if (pRevData[data_idx + data_len] != PROTOCOL_TAIL_0 || 
        pRevData[data_idx + data_len + 1] != PROTOCOL_TAIL_1)
    {
        return -1;
    }
    
    // Parse according to module and command
    switch (module)
    {
        case PROTOCOL_MODULE_ULTRASOUND:
            switch (cmd)
            {
                case PROTOCOL_CMD_GET_STATUS:
                {
                    US_GetStatus_Reply_t *pStatus = (US_GetStatus_Reply_t *)&s_AppCommInfo.RevData;
                    uint8_t idx = data_idx;
                    
                    if (data_len != 12)
                    {
                        return -1;
                    }
                    
                    pStatus->work_state = pRevData[idx++];
                    idx = App_Comm_ReadU16(pRevData, idx, &pStatus->frequency);
                    idx = App_Comm_ReadU16(pRevData, idx, &pStatus->temp_limit);
                    idx = App_Comm_ReadU16(pRevData, idx, &pStatus->remain_time);
                    pStatus->work_level = pRevData[idx++];
                    idx = App_Comm_ReadU16(pRevData, idx, &pStatus->head_temp);
                    pStatus->conn_state = pRevData[idx++];
                    pStatus->error_code = pRevData[idx++];
                    break;
                }
                
                case PROTOCOL_CMD_SET_CONFIG:
                {
                    US_SetConfig_Reply_t *pConfig = (US_SetConfig_Reply_t *)&s_AppCommInfo.RevData;
                    uint8_t idx = data_idx;
                    
                    if (data_len != 3)
                    {
                        return -1;
                    }
                    
                    pConfig->freq_result = pRevData[idx++];
                    pConfig->voltage_result = pRevData[idx++];
                    pConfig->temp_result = pRevData[idx++];
                    break;
                }
                
                default:
                    return -1;
            }
            break;
            
        case PROTOCOL_MODULE_RADIO_FREQ:
            switch (cmd)
            {
                case PROTOCOL_CMD_GET_STATUS:
                {
                    RF_GetStatus_Reply_t *pStatus = (RF_GetStatus_Reply_t *)&s_AppCommInfo.RevData;
                    uint8_t idx = data_idx;
                    
                    if (data_len != 10)
                    {
                        return -1;
                    }
                    
                    pStatus->work_state = pRevData[idx++];
                    idx = App_Comm_ReadU16(pRevData, idx, &pStatus->temp_limit);
                    idx = App_Comm_ReadU16(pRevData, idx, &pStatus->remain_time);
                    pStatus->work_level = pRevData[idx++];
                    idx = App_Comm_ReadU16(pRevData, idx, &pStatus->head_temp);
                    pStatus->conn_state = pRevData[idx++];
                    pStatus->error_code = pRevData[idx++];
                    break;
                }
                
                case PROTOCOL_CMD_SET_CONFIG:
                {
                    RF_SetConfig_Reply_t *pConfig = (RF_SetConfig_Reply_t *)&s_AppCommInfo.RevData;
                    uint8_t idx = data_idx;
                    
                    if (data_len != 1)
                    {
                        return -1;
                    }
                    
                    pConfig->temp_result = pRevData[idx++];
                    break;
                }
                
                default:
                    return -1;
            }
            break;
            
        case PROTOCOL_MODULE_SHOCKWAVE:
            switch (cmd)
            {
                case PROTOCOL_CMD_GET_STATUS:
                {
                    SW_GetStatus_Reply_t *pStatus = (SW_GetStatus_Reply_t *)&s_AppCommInfo.RevData;
                    uint8_t idx = data_idx;
                    
                    if (data_len != 9)
                    {
                        return -1;
                    }
                    
                    pStatus->work_state = pRevData[idx++];
                    pStatus->frequency = pRevData[idx++];
                    idx = App_Comm_ReadU16(pRevData, idx, &pStatus->remain_time);
                    pStatus->work_level = pRevData[idx++];
                    idx = App_Comm_ReadU16(pRevData, idx, &pStatus->head_temp);
                    pStatus->conn_state = pRevData[idx++];
                    pStatus->error_code = pRevData[idx++];
                    break;
                }
                
                default:
                    return -1;
            }
            break;
            
        case PROTOCOL_MODULE_HEAT:
            switch (cmd)
            {
                case PROTOCOL_CMD_GET_STATUS:
                {
                    Heat_GetStatus_Reply_t *pStatus = (Heat_GetStatus_Reply_t *)&s_AppCommInfo.RevData;
                    uint8_t idx = data_idx;
                    
                    if (data_len != 17)
                    {
                        return -1;
                    }
                    
                    pStatus->work_state = pRevData[idx++];
                    idx = App_Comm_ReadU16(pRevData, idx, &pStatus->temp_limit);
                    idx = App_Comm_ReadU16(pRevData, idx, &pStatus->remain_heat_time);
                    pStatus->suck_time = pRevData[idx++];
                    pStatus->release_time = pRevData[idx++];
                    pStatus->pressure = pRevData[idx++];
                    idx = App_Comm_ReadU16(pRevData, idx, &pStatus->head_temp);
                    pStatus->preheat_state = pRevData[idx++];
                    idx = App_Comm_ReadU16(pRevData, idx, &pStatus->preheat_temp_limit);
                    idx = App_Comm_ReadU16(pRevData, idx, &pStatus->remain_preheat_time);
                    pStatus->conn_state = pRevData[idx++];
                    pStatus->error_code = pRevData[idx++];
                    break;
                }
                
                default:
                    return -1;
            }
            break;
            
        default:
            return -1;
    }
    
    return 0;
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
