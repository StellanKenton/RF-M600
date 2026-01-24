/************************************************************************************
* @file     : drv_adc.h
* @brief    : ADC driver header file
* @details  : ADC channel enumeration and read functions
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef DRV_ADC_H
#define DRV_ADC_H

#include "main.h"
#include "adc.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
#include <iostream>
extern "C" {
#endif

/**
 * @brief ADC Channel enumeration (ADC通道枚举)
 */
typedef enum
{
    E_ADC_CHANNEL_US_I = 0,          ///< ADC1_IN0  - PA0
    E_ADC_CHANNEL_RF_I,              ///< ADC1_IN1  - PA1
    E_ADC_CHANNEL_Heat_REF02,       ///< ADC1_IN5  - PA5
    E_ADC_CHANNEL_Heat_REF01,       ///< ADC1_IN6  - PA6
    E_ADC_CHANNEL_ESW_U,              ///< ADC1_IN8  - PB0
    E_ADC_CHANNEL_ESW_I,              ///< ADC1_IN9  - PB1
    E_ADC_CHANNEL_HP_PRE,             ///< ADC1_IN12 - PC2
    E_ADC_CHANNEL_HAND_NTC,             ///< ADC1_IN13 - PC3
    E_ADC_CHANNEL_VER_ID,             ///< ADC1_IN14 - PC4
    E_ADC_CHANNEL_VOUT,             ///< ADC1_IN15 - PC5
    E_ADC_CHANNEL_MAX             ///< ADC通道数量
} ADC_Channel_EnumDef;

/**
 * @brief Read ADC value from specified channel
 * @param channel ADC channel enumeration
 * @retval ADC value (0-4095 for 12-bit ADC), returns 0 if error
 */
uint16_t Drv_ADC_ReadChannel(ADC_Channel_EnumDef channel);

/**
 * @brief Read ADC value with voltage conversion (assuming 3.3V reference)
 * @param channel ADC channel enumeration
 * @retval Voltage in millivolts (mV), returns 0 if error
 */
uint32_t Drv_ADC_ReadVoltage(ADC_Channel_EnumDef channel);

/**
 * @brief Read VOUT voltage (DAC output voltage monitoring)
 * @retval Voltage in millivolts (mV)
 */
uint16_t Drv_ADC_ReadVOUT(void);



#ifdef __cplusplus
}
#endif
#endif  // DRV_ADC_H
/**************************End of file********************************/
