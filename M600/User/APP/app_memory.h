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
    uint16_t TempLimit;         ///< Temperature limit in 0.1°C (温度限制，35-48℃)
    uint16_t RemainTimes;       ///< Remaining treatment times (剩余可治疗次数)
    uint16_t CurrentHigh;       ///< Current high limit in mV (工作电流上限，采样电压)
    uint16_t CurrentLow;        ///< Current low limit in mV (工作电流下限，采样电压)
    uint16_t CrcCode;           ///< CRC code (CRC校验码)
} RF_TreatParams_t;

/**
 * @brief Shock Wave treatment parameters structure
 */
typedef struct
{
    uint16_t Energy;            ///< Energy level (能量等级)
    uint16_t Frequency;         ///< Frequency in Hz (频率)
    uint16_t WorkTime;          ///< Working time in seconds (工作时间)
    uint16_t PulseCount;        ///< Pulse count (脉冲计数)
    uint16_t Voltage;           ///< Voltage in mV (电压)
    uint16_t Current;           ///< Current in mA (电流)
    uint8_t WorkLevel;          ///< Working level (工作等级)
    uint8_t Reserved[1];        ///< Reserved byte (保留字节)
    uint16_t CrcCode;           ///< CRC code (CRC校验码)
} SW_TreatParams_t;

/**
 * @brief Negative Pressure Heat treatment parameters structure
 */
typedef struct
{
    uint16_t Pressure;          ///< Pressure level (压力等级)
    uint16_t Temperature;       ///< Temperature in 0.1°C (温度)
    uint16_t WorkTime;          ///< Working time in seconds (工作时间)
    uint16_t VacuumLevel;       ///< Vacuum level (真空度)
    uint16_t HeatPower;         ///< Heat power in mW (加热功率)
    uint16_t TempLimit;         ///< Temperature limit in 0.1°C (温度限制)
    uint8_t WorkLevel;          ///< Working level (工作等级)
    uint8_t Reserved[1];        ///< Reserved byte (保留字节)
    uint16_t CrcCode;           ///< CRC code (CRC校验码)
} NPH_TreatParams_t;

/**
 * @brief Ultrasound treatment parameters structure
 */
typedef struct
{
    uint16_t Frequency;         ///< Frequency in Hz (频率)
    uint16_t TempLimit;         ///< Temperature limit in 0.1°C (温度限制)
    uint16_t Voltage;           ///< Voltage in mV (电压)
    uint16_t CurrentHigh;       ///< Current in mA (电流)
    uint16_t CurrentLow;        ///< Current in mA (电流)
    uint16_t RemainTimes;       ///< Remaining treatment times (次数)
    uint16_t CrcCode;           ///< CRC code (CRC校验码)
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
