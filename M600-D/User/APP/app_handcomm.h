/***********************************************************************************
* @file     : app_handcomm.h
* @brief    : Handle communication module - receive temperature from handle
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2025
**********************************************************************************/
#ifndef APP_HANDCOMM_H
#define APP_HANDCOMM_H

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =============================================================================
 * Constants
 * ============================================================================= */
#define HANDCOMM_TASK_TIME       5       ///< Task period in ms
#define HANDCOMM_RX_BUFFER_SIZE  128     ///< Receive buffer size

/* Protocol Constants */
#define HANDCOMM_HEADER_0        0x5A
#define HANDCOMM_HEADER_1        0xA5
#define HANDCOMM_DIR_HANDLE_TO_DEV 0x00  ///< Handle to Device
#define HANDCOMM_MODULE_HANDLE    0x05   ///< Handle Module

/* Command Code */
#define HANDCOMM_CMD_TEMP_REPORT  0x00   ///< Temperature report from handle

/* =============================================================================
 * Data Structures
 * ============================================================================= */

/* Handle Temperature Report */
typedef struct
{
    uint16_t temperature;        ///< Temperature value (value/10 = actual temperature in ℃)
    uint8_t sensor_id;           ///< Sensor ID (if multiple sensors)
    uint8_t status;              ///< Status: 0x00=OK, 0xFF=Error
} HandComm_TempReport_t;

/* Handle Communication Info */
typedef struct
{
    uint8_t RxData[128];         ///< Receive buffer
    uint8_t TxData[128];         ///< Transmit buffer
    HandComm_TempReport_t TempData;  ///< Latest temperature data
    bool TempDataValid;          ///< Temperature data valid flag
    uint32_t LastUpdateTime;     ///< Last update time (ms)
} HandComm_Info_t;

/* =============================================================================
 * Function Prototypes
 * ============================================================================= */

/* Initialization and Process */
void App_HandComm_Init(void);
void App_HandComm_Process(void);

/* Get Temperature Data */
HandComm_TempReport_t* App_HandComm_GetTempData(void);
bool App_HandComm_IsTempDataValid(void);

#ifdef __cplusplus
}
#endif

#endif  // APP_HANDCOMM_H
/**************************End of file********************************/
