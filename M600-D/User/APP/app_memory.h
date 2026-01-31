/************************************************************************************
* @file     : app_memory.h
* @brief    : Treatment parameters memory management module
* @details  : Define parameter structures for four treatment modes
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef APP_MEMORY_H
#define APP_MEMORY_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
#include <iostream>
extern "C" {
#endif

/**
 * @brief Radio Frequency treatment parameters structure
 */
typedef struct
{
    uint16_t TempLimit;         ///< Temperature limit in 0.1C (35-48C)
    uint16_t TreatRemainTimes;  ///< Remaining treatment times
    uint16_t CurrentHigh;       ///< Current high limit in mV (sampled voltage)
    uint16_t CurrentLow;        ///< Current low limit in mV (sampled voltage)
    uint16_t CrcCode;           ///< CRC checksum
} RF_TreatParams_t;

/**
 * @brief Shock Wave treatment parameters structure
 */
typedef struct
{
    uint16_t TempLimit;         ///< Temperature limit in 0.1C (35-48C)
    uint16_t TreatRemainTimes;  ///< Remaining treatment times
    uint16_t CurrentHigh_ESW_P; ///< PWM_ESW+ work current high limit in mV
    uint16_t CurrentLow_ESW_P;  ///< PWM_ESW+ work current low limit in mV
    uint16_t CurrentHigh_ESW_N; ///< PWM_ESW- work current high limit in mV
    uint16_t CurrentLow_ESW_N;  ///< PWM_ESW- work current low limit in mV
    uint16_t CrcCode;           ///< CRC checksum
} SW_TreatParams_t;

/**
 * @brief Negative Pressure Heat treatment parameters structure
 */
typedef struct
{
    uint16_t TempLimit;         ///< Temperature limit in 0.1C (35-48C)
    uint16_t TreatRemainTimes;  ///< Remaining treatment times
    uint8_t PreheatEnable;      ///< Preheat enable (0=off, 1=on)
    uint16_t PreheatTempLimit;  ///< Preheat temperature limit in 0.1C (35-48C)
    uint16_t PreheatTime;       ///< Preheat time in seconds
    uint16_t CrcCode;           ///< CRC checksum
} NPH_TreatParams_t;

/**
 * @brief Ultrasound treatment parameters structure
 */
typedef struct
{
    uint16_t Frequency;         ///< Frequency in Hz
    uint16_t TempLimit;         ///< Temperature limit in 0.1C
    uint16_t Voltage;           ///< Voltage in mV
    uint16_t CurrentHigh;       ///< Current high limit in mA
    uint16_t CurrentLow;        ///< Current low limit in mA
    uint16_t TreatRemainTimes;  ///< Remaining treatment times
    uint16_t CrcCode;           ///< CRC checksum
} US_TreatParams_t;

/**
 * @brief All treatment parameters union
 */
typedef union
{
    RF_TreatParams_t rfParams;      ///< Radio Frequency parameters
    SW_TreatParams_t swParams;      ///< Shock Wave parameters
    NPH_TreatParams_t nphParams;    ///< Negative Pressure Heat parameters
    US_TreatParams_t usParams;      ///< Ultrasound parameters
    uint8_t rawData[16];            ///< Raw data buffer
} TreatParams_Union_t;

/**
 * @brief Initialize memory module
 */
void App_Memory_Init(void);

/**
 * @brief Save Radio Frequency treatment parameters
 * @param params Pointer to RF treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_SaveRFParams(const RF_TreatParams_t *params);

/**
 * @brief Load Radio Frequency treatment parameters
 * @param params Pointer to store loaded RF treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_LoadRFParams(RF_TreatParams_t *params);

/**
 * @brief Save Shock Wave treatment parameters
 * @param params Pointer to SW treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_SaveSWParams(const SW_TreatParams_t *params);

/**
 * @brief Load Shock Wave treatment parameters
 * @param params Pointer to store loaded SW treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_LoadSWParams(SW_TreatParams_t *params);

/**
 * @brief Save Negative Pressure Heat treatment parameters
 * @param params Pointer to NPH treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_SaveNPHParams(const NPH_TreatParams_t *params);

/**
 * @brief Load Negative Pressure Heat treatment parameters
 * @param params Pointer to store loaded NPH treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_LoadNPHParams(NPH_TreatParams_t *params);

/**
 * @brief Save Ultrasound treatment parameters
 * @param params Pointer to US treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_SaveUSParams(const US_TreatParams_t *params);

/**
 * @brief Load Ultrasound treatment parameters
 * @param params Pointer to store loaded US treatment parameters
 * @retval true if success, false if failed
 */
bool App_Memory_LoadUSParams(US_TreatParams_t *params);

#ifdef __cplusplus
}
#endif
#endif  // APP_MEMORY_H
/**************************End of file********************************/
