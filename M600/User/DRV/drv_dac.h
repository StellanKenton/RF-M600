/************************************************************************************
* @file     : drv_dac.h
* @brief    : DAC driver header file
* @details  : DAC voltage output control functions
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef DRV_DAC_H
#define DRV_DAC_H

#include "main.h"
#include "dac.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
#include <iostream>
extern "C" {
#endif

/**
 * @brief Set DAC output voltage
 * @param voltage_mv Voltage in millivolts (mV), range: 0-3300mV
 * @retval true if success, false if failed
 */
bool Drv_DAC_SetVoltage(uint16_t voltage_mv);

/**
 * @brief Get current DAC output voltage
 * @retval Current voltage in millivolts (mV)
 */
uint16_t Drv_DAC_GetVoltage(void);

/**
 * @brief Initialize DAC driver
 */
void Drv_DAC_Init(void);

#ifdef __cplusplus
}
#endif
#endif  // DRV_DAC_H
/**************************End of file********************************/
