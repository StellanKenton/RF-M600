/***********************************************************************************
* @file     : app_handcomm.c
* @brief    : Handle communication module - receive temperature from handle
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2025
**********************************************************************************/
#include "app_handcomm.h"
#include "lib_ringbuffer.h"
#include "drv_usart.h"
#include "drv_delay.h"
#include <string.h>

static HandComm_Info_t s_HandCommInfo;

/* =============================================================================
 * Private Functions
 * ============================================================================= */

/**
 * @brief Calculate CRC16 checksum
 * @param data Pointer to data buffer
 * @param length Data length
 * @retval CRC16 value
 */
static uint16_t HandComm_Crc16Compute(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0x0000;
    
    while (length--) {
        uint8_t b = *data++;
        
        // Input bit reversal
        uint8_t r = 0;
        for (uint8_t i = 0; i < 8; i++) {
            r = (r << 1) | (b & 0x01);
            b >>= 1;
        }
        
        crc ^= (uint16_t)r << 8;
        
        // Process 8 bits
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x8005;
            } else {
                crc <<= 1;
            }
        }
    }
    
    // Output bit reversal
    uint16_t result = 0;
    for (uint8_t i = 0; i < 16; i++) {
        result = (result << 1) | (crc & 0x01);
        crc >>= 1;
    }
    
    return result;
}

/**
 * @brief Parse temperature report frame
 * @param pData Pointer to data buffer
 * @param dataLen Data length
 * @retval true if parse success, false otherwise
 */
static bool HandComm_ParseTempReport(const uint8_t *pData, uint8_t dataLen)
{
    if (pData == NULL || dataLen < 4) {
        return false;
    }
    
    // Parse temperature data: [temp_low, temp_high, sensor_id, status]
    s_HandCommInfo.TempData.temperature = pData[0] | ((uint16_t)pData[1] << 8);
    s_HandCommInfo.TempData.sensor_id = pData[2];
    s_HandCommInfo.TempData.status = pData[3];
    s_HandCommInfo.TempDataValid = true;
    
    return true;
}

/**
 * @brief Handle received data frame
 * @param pRxFrame Pointer to received frame data
 * @param frameLen Frame length
 */
static void HandComm_RecvDataHandle(const uint8_t *pRxFrame, uint16_t frameLen)
{
    if (pRxFrame == NULL || frameLen < 8) {
        return;
    }
    
    // Check header
    if (pRxFrame[0] != HANDCOMM_HEADER_0 || 
        pRxFrame[1] != HANDCOMM_HEADER_1 ||
        pRxFrame[2] != HANDCOMM_DIR_HANDLE_TO_DEV) {
        return;
    }
    
    // Check module
    if (pRxFrame[3] != HANDCOMM_MODULE_HANDLE) {
        return;
    }
    
    uint8_t cmd = pRxFrame[4];
    uint8_t dataLen = pRxFrame[5];
    
    // Check frame length
    if (frameLen < (dataLen + 8)) {
        return;
    }
    
    // Verify CRC16
    uint16_t calculatedCrc = HandComm_Crc16Compute(pRxFrame + 6, dataLen);
    uint16_t receivedCrc = pRxFrame[6 + dataLen] | ((uint16_t)pRxFrame[6 + dataLen + 1] << 8);
    
    if (calculatedCrc != receivedCrc) {
        return;
    }
    
    // Process command
    switch (cmd) {
        case HANDCOMM_CMD_TEMP_REPORT:
            HandComm_ParseTempReport(pRxFrame + 6, dataLen);
            break;
        default:
            break;
    }
}

/**
 * @brief Receive data from USART2 ring buffer
 */
static void App_HandComm_RecvData(void)
{
    static uint8_t UartRxData[HANDCOMM_RX_BUFFER_SIZE];
    static uint16_t OverTime = 0;
    
    CBuff* pRxBuffer = Drv_GetUsart2RingPtr();
    if (pRxBuffer == NULL) {
        return;
    }
    
    if (CBuff_GetLength(pRxBuffer) < 6) {
        return;
    }
    
    // Read header and basic info
    CBuff_Read(pRxBuffer, UartRxData, 6);
    
    // Check header
    if (UartRxData[0] != HANDCOMM_HEADER_0 || 
        UartRxData[1] != HANDCOMM_HEADER_1 || 
        UartRxData[2] != HANDCOMM_DIR_HANDLE_TO_DEV) {
        CBuff_Pop(pRxBuffer, UartRxData, 1);
        return;
    }
    
    uint8_t dataLen = UartRxData[5];
    
    // Check if complete frame is available
    if (CBuff_GetLength(pRxBuffer) < (dataLen + 8)) {
        OverTime += HANDCOMM_TASK_TIME;
        if (OverTime >= 200) {
            OverTime = 0;
            CBuff_Pop(pRxBuffer, UartRxData, 1);
        }
        return;
    }
    
    OverTime = 0;
    
    // Read complete frame
    CBuff_Read(pRxBuffer, UartRxData, dataLen + 8);
    
    // Process frame
    HandComm_RecvDataHandle(UartRxData, dataLen + 8);
    
    // Remove processed data from buffer
    CBuff_Pop(pRxBuffer, UartRxData, dataLen + 8);
}

/* =============================================================================
 * Public Functions
 * ============================================================================= */

/**
 * @brief Initialize handle communication module
 */
void App_HandComm_Init(void)
{
    memset(&s_HandCommInfo, 0, sizeof(HandComm_Info_t));
    s_HandCommInfo.TempDataValid = false;
}

/**
 * @brief Process handle communication (called periodically)
 */
void App_HandComm_Process(void)
{
    static Drv_Timer_t HandCommTimer;
    if (Drv_Timer_Tick(&HandCommTimer, HANDCOMM_TASK_TIME) == false) {
        return;
    }
    
    App_HandComm_RecvData();
}

/**
 * @brief Get temperature data
 * @retval Pointer to temperature data structure
 */
HandComm_TempReport_t* App_HandComm_GetTempData(void)
{
    return &s_HandCommInfo.TempData;
}

/**
 * @brief Check if temperature data is valid
 * @retval true if valid, false otherwise
 */
bool App_HandComm_IsTempDataValid(void)
{
    return s_HandCommInfo.TempDataValid;
}

/**************************End of file********************************/
