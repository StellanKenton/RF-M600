/************************************************************************************
* @file     : app_comm.h
* @brief    : 
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef APP_COMM_H
#define APP_COMM_H

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
#include <iostream>
extern "C" {
#endif

/* =============================================================================
 * Protocol Constants
 * ============================================================================= */
#define PROTOCOL_HEADER_0          0x5A
#define PROTOCOL_HEADER_1          0xA5
#define PROTOCOL_TAIL_0            0xC3
#define PROTOCOL_TAIL_1            0x3C

/* Transmission Direction */
#define PROTOCOL_DIR_HOST_TO_DEV   0x00    ///< Host to Device
#define PROTOCOL_DIR_DEV_TO_HOST   0x01    ///< Device to Host

/* Module Command */
#define PROTOCOL_MODULE_ULTRASOUND     0x01    ///< Ultrasound Module
#define PROTOCOL_MODULE_RADIO_FREQ     0x02    ///< Radio Frequency Module
#define PROTOCOL_MODULE_SHOCKWAVE      0x03    ///< Shockwave Module
#define PROTOCOL_MODULE_HEAT           0x04    ///< Heat Therapy Module

/* Command Code */
#define PROTOCOL_CMD_GET_STATUS        0x00    ///< Get Device Status
#define PROTOCOL_CMD_SET_WORK_STATE    0x01    ///< Set Working State
#define PROTOCOL_CMD_SET_CONFIG       0x02    ///< Set Internal Configuration

/* Work State */
#define WORK_STATE_STOP               0x00    ///< Stop
#define WORK_STATE_START              0x01    ///< Start/Working
#define WORK_STATE_RESET              0x02    ///< Reset

/* Connection Status */
#define CONN_STATE_CONNECTED_FOOT_CLOSED      0x00    ///< Head connected, foot closed
#define CONN_STATE_DISCONNECTED_FOOT_CLOSED   0x10    ///< Head disconnected, foot closed
#define CONN_STATE_CONNECTED_FOOT_OPEN        0x01    ///< Head connected, foot open
#define CONN_STATE_DISCONNECTED_FOOT_OPEN     0x11    ///< Head disconnected, foot open

/* Temperature Error Codes */
#define TEMP_ERROR_NTC_OPEN           0xFFFF  ///< NTC open circuit
#define TEMP_ERROR_NTC_SHORT          0xEEFF  ///< NTC short circuit
#define TEMP_ERROR_OVER_LIMIT         0xFFFF  ///< Over limit (for frequency/voltage)

/* Config Result */
#define CONFIG_RESULT_SUCCESS         0x00    ///< Success
#define CONFIG_RESULT_FAIL            0x01    ///< Fail
#define CONFIG_RESULT_OVER_LIMIT      0x02    ///< Over limit

/* =============================================================================
 * Protocol Frame Structure
 * ============================================================================= */
typedef struct
{
    uint8_t header[2];           ///< Fixed header: 0x5A 0xA5
    uint8_t direction;           ///< Transmission direction
    uint8_t module;              ///< Module command
    uint8_t cmd;                 ///< Command code
    uint8_t data_len;            ///< Data field length
    uint8_t *data;               ///< Data field pointer
    uint8_t tail[2];             ///< Fixed tail: 0xC3 0x3C
} Protocol_Frame_t;

/* =============================================================================
 * Ultrasound Module Structures
 * ============================================================================= */

/* Ultrasound - Get Status (0x00) - Send */
typedef struct
{
    uint8_t dummy;               ///< Always 0x00
} US_GetStatus_Send_t;

/* Ultrasound - Get Status (0x00) - Reply */
typedef struct
{
    uint8_t work_state;          ///< 0x00: Stop, 0x01: Working
    uint16_t frequency;          ///< 1000-1400 (kHz)
    uint16_t temp_limit;         ///< 350-480 (35-48℃), 0xFFFF: Over limit
    uint16_t remain_time;        ///< Remaining work time (seconds), max 3600
    uint8_t work_level;          ///< Work level: 0-39 (40 levels)
    uint16_t head_temp;          ///< Head temperature = value/10, 0xFFFF: NTC open, 0xEEFF: NTC short
    uint8_t conn_state;          ///< Connection status
    uint8_t error_code;          ///< Reserved error code
} US_GetStatus_Reply_t;

/* Ultrasound - Set Work State (0x01) - Send */
typedef struct
{
    uint8_t work_state;          ///< 0x01: Start, 0x00: Stop, 0x02: Reset
    uint16_t work_time;          ///< Work time (seconds), max 3600
    uint8_t work_level;          ///< Work level: 0-39 (40 levels)
} US_SetWorkState_Send_t;

/* Ultrasound - Set Config (0x02) - Send */
typedef struct
{
    uint16_t frequency;          ///< 1000-1400 (kHz)
    uint16_t voltage;            ///< 1000-2000 (10-20V)
    uint16_t temp_limit;         ///< 350-480 (35-48℃)
} US_SetConfig_Send_t;

/* Ultrasound - Set Config (0x02) - Reply */
typedef struct
{
    uint8_t freq_result;         ///< 0x00: Success, 0x01: Fail, 0x02: Over limit
    uint8_t voltage_result;      ///< 0x00: Success, 0x01: Fail, 0x02: Over limit
    uint8_t temp_result;         ///< 0x00: Success, 0x01: Fail, 0x02: Over limit
} US_SetConfig_Reply_t;

typedef union {
    uint16_t byte;  
    struct {
        uint16_t Rely_Status : 1;
        uint16_t Rely_Config : 1;
        uint16_t Reserved : 14;
    } bits;
} UltraSound_ByteUnion;

typedef struct
{
    UltraSound_ByteUnion flag;
    US_GetStatus_Reply_t TxStatus;
    US_SetConfig_Reply_t TxConfig;

    US_SetWorkState_Send_t RxWorkState;
    US_SetConfig_Send_t RxConfig; 
    bool RxValidFlag[3];

} UltraSound_TransData_t;

UltraSound_TransData_t *App_Comm_GetUSTransData(void);

/* =============================================================================
 * Radio Frequency Module Structures
 * ============================================================================= */

/* RF - Get Status (0x00) - Send */
typedef struct
{
    uint8_t dummy;               ///< Always 0x00
} RF_GetStatus_Send_t;

/* RF - Get Status (0x00) - Reply */
typedef struct
{
    uint8_t work_state;          ///< 0x00: Stop, 0x01: Working
    uint16_t temp_limit;         ///< 350-480 (35-48℃), 0xFFFF: Over limit
    uint16_t remain_time;        ///< Remaining work time (seconds), max 3600
    uint8_t work_level;          ///< Work level: 0-20
    uint16_t head_temp;          ///< Head temperature = value/10, 0xFFFF: NTC open, 0xEEFF: NTC short
    uint8_t conn_state;          ///< Connection status
    uint8_t error_code;          ///< Reserved error code
} RF_GetStatus_Reply_t;

/* RF - Set Work State (0x01) - Send */
typedef struct
{
    uint8_t work_state;          ///< 0x01: Start, 0x00: Stop, 0x02: Reset
    uint16_t work_time;          ///< Work time (seconds), max 3600
    uint8_t work_level;          ///< Work level: 0-39 (40 levels)
} RF_SetWorkState_Send_t;

/* RF - Set Config (0x02) - Send */
typedef struct
{
    uint16_t temp_limit;         ///< 350-480 (35-48℃)
} RF_SetConfig_Send_t;

/* RF - Set Config (0x02) - Reply */
typedef struct
{
    uint8_t temp_result;         ///< 0x00: Success, 0x01: Fail, 0x02: Over limit
} RF_SetConfig_Reply_t;

typedef union {
    uint16_t byte;  
    struct {
        uint16_t Rely_Status : 1;
        uint16_t Rely_Config : 1;
        uint16_t Reserved : 14;
    } bits;
} RF_ByteUnion;

typedef struct
{
    RF_ByteUnion flag;
    RF_GetStatus_Reply_t TxStatus;
    RF_SetWorkState_Send_t RxWorkState;
    RF_SetConfig_Send_t RxConfig;
    RF_SetConfig_Reply_t TxConfig;
} RF_TransData_t;

RF_TransData_t *App_Comm_GetRFTransData(void);
/* =============================================================================
 * Shockwave Module Structures
 * ============================================================================= */

/* Shockwave - Get Status (0x00) - Send */
typedef struct
{
    uint8_t dummy;               ///< Always 0x00
} SW_GetStatus_Send_t;

/* Shockwave - Get Status (0x00) - Reply */
typedef struct
{
    uint8_t work_state;          ///< 0x00: Stop, 0x01: Working
    uint8_t frequency;           ///< Frequency: 1-16 levels
    uint16_t remain_time;        ///< Remaining work time (seconds), max 3600
    uint8_t work_level;          ///< Work level: 0-20
    uint16_t head_temp;          ///< Head temperature = value/10, 0xFFFF: NTC open, 0xEEFF: NTC short
    uint8_t conn_state;          ///< Connection status
    uint8_t error_code;          ///< Reserved error code
} SW_GetStatus_Reply_t;

/* Shockwave - Set Work State (0x01) - Send */
typedef struct
{
    uint8_t work_state;          ///< 0x01: Start, 0x00: Stop, 0x02: Reset
    uint16_t work_time;          ///< Work time (seconds), max 3600
    uint8_t work_level;          ///< Work level: 0-26
    uint8_t frequency;           ///< Frequency: 0-16
} SW_SetWorkState_Send_t;

typedef union {
    uint16_t byte;  
    struct {
        uint16_t Rely_Status : 1;
        uint16_t Reserved : 15;
    } bits;
} SW_ByteUnion;

typedef struct
{
    SW_ByteUnion flag;
    SW_GetStatus_Reply_t TxStatus;
    SW_SetWorkState_Send_t RxWorkState;
} SW_TransData_t;

SW_TransData_t *App_Comm_GetSWTransData(void);
/* =============================================================================
 * Heat Therapy Module Structures
 * ============================================================================= */

/* Heat - Get Status (0x00) - Send */
typedef struct
{
    uint8_t dummy;               ///< Always 0x00
} Heat_GetStatus_Send_t;

/* Heat - Get Status (0x00) - Reply */
typedef struct
{
    uint8_t work_state;          ///< 0x00: Stop, 0x01: Working
    uint16_t temp_limit;         ///< 350-480 (35-48℃), 0xFFFF: Over limit
    uint16_t remain_heat_time;   ///< Remaining heat time (seconds), max 3600
    uint16_t suck_time;            ///< Suck time: unit 10ms, 10-60000
    uint16_t release_time;        ///< Release time: unit 10ms, 10-60000
    uint8_t pressure;            ///< Pressure: 10-100 KPa
    uint16_t head_temp;          ///< Head temperature = value/10, 0xFFFF: NTC open, 0xEEFF: NTC short
    uint8_t preheat_state;       ///< Preheat state: 0x00: Stop, 0x01: Working
    uint16_t preheat_temp_limit; ///< Preheat temperature limit: 350-480 (35-48℃)
    uint16_t remain_preheat_time; ///< Remaining preheat time (seconds), max 3600
    uint8_t conn_state;          ///< Connection status
    uint8_t error_code;          ///< Reserved error code
} Heat_GetStatus_Reply_t;

/* Heat - Set Work State (0x01) - Send */
typedef struct
{
    uint8_t work_state;          ///< 0x01: Start, 0x00: Stop, 0x02: Reset
    uint16_t work_time;          ///< Work time (seconds), max 3600
    uint8_t pressure;            ///< Pressure: -10KPa to -100KPa (send positive value)
    uint16_t suck_time;           ///< Suck time: unit 100ms, 0.1s-60s
    uint16_t release_time;        ///< Release time: unit 100ms, 0.1s-60s
    uint16_t temp_limit;         ///< 350-480 (35-48℃)
} Heat_SetWorkState_Send_t;

/* Heat - Set Config/Preheat (0x02) - Send */
typedef struct
{
    uint8_t preheat_state;       ///< 0x01: Start, 0x00: Stop (only when heat therapy stopped)
    uint16_t work_time;          ///< Work time (seconds), max 3600
    uint16_t temp_limit;          ///< 350-480 (35-48℃)
} Heat_SetPreheat_Send_t;


typedef union {
    uint16_t byte;  
    struct {
        uint16_t Rely_Status : 1;
        uint16_t Rely_Config : 1;
        uint16_t Reserved : 14;
    } bits;
} Heat_ByteUnion;

typedef struct
{
    Heat_ByteUnion flag;
    Heat_GetStatus_Reply_t TxStatus;
    Heat_SetWorkState_Send_t RxWorkState;
    Heat_SetPreheat_Send_t RxPreheat;
} Heat_TransData_t;

Heat_TransData_t *App_Comm_GetHeatTransData(void);

typedef struct
{
    uint8_t RxData[128];
    uint8_t TxData[128];
    UltraSound_TransData_t US;
    RF_TransData_t RF;
    SW_TransData_t SW;
    Heat_TransData_t Heat;
} App_Comm_Info_t;




/* =============================================================================
 * Function Prototypes
 * ============================================================================= */

/* Initialization and Process */
void App_Comm_Init(void);
void App_Comm_Process(void);

/* Send Packet Build Functions */
int8_t App_Comm_BuildUS_GetStatus_Send(const US_GetStatus_Send_t *pData, uint8_t *pTxData, uint8_t *pLen);
int8_t App_Comm_BuildUS_SetWorkState_Send(const US_SetWorkState_Send_t *pData, uint8_t *pTxData, uint8_t *pLen);
int8_t App_Comm_BuildUS_SetConfig_Send(const US_SetConfig_Send_t *pData, uint8_t *pTxData, uint8_t *pLen);

int8_t App_Comm_BuildRF_GetStatus_Send(const RF_GetStatus_Send_t *pData, uint8_t *pTxData, uint8_t *pLen);
int8_t App_Comm_BuildRF_SetWorkState_Send(const RF_SetWorkState_Send_t *pData, uint8_t *pTxData, uint8_t *pLen);
int8_t App_Comm_BuildRF_SetConfig_Send(const RF_SetConfig_Send_t *pData, uint8_t *pTxData, uint8_t *pLen);

int8_t App_Comm_BuildSW_GetStatus_Send(const SW_GetStatus_Send_t *pData, uint8_t *pTxData, uint8_t *pLen);
int8_t App_Comm_BuildSW_SetWorkState_Send(const SW_SetWorkState_Send_t *pData, uint8_t *pTxData, uint8_t *pLen);

int8_t App_Comm_BuildHeat_GetStatus_Send(const Heat_GetStatus_Send_t *pData, uint8_t *pTxData, uint8_t *pLen);
int8_t App_Comm_BuildHeat_SetWorkState_Send(const Heat_SetWorkState_Send_t *pData, uint8_t *pTxData, uint8_t *pLen);
int8_t App_Comm_BuildHeat_SetPreheat_Send(const Heat_SetPreheat_Send_t *pData, uint8_t *pTxData, uint8_t *pLen);

/* Receive Parse Function */
int8_t App_Comm_ParseReceive(const uint8_t *pRevData, uint8_t revLen);

#ifdef __cplusplus
}
#endif
#endif  // APP_COMM_H
/**************************End of file********************************/
