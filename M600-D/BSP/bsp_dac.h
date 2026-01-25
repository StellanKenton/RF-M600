/************************************************************************************
 * @file     : bsp_dac.h
 * @brief    : M600 DAC module - DAC Ch1 on PA4 (STM32 Standard Library)
 * @details  : Ported from M600 HAL. Trigger none, output buffer enable.
 * @hardware : STM32F103xE (M600)
 ***********************************************************************************/
#ifndef __BSP_DAC_H
#define __BSP_DAC_H

#include "stm32f10x.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void BSP_DAC_Init(void);
void BSP_DAC_SetValue(uint16_t value);   /* 12-bit, 0..4095 */
void BSP_DAC_SetVoltage(uint16_t mv);    /* 0..3300 mV, 3.3V ref */

#ifdef __cplusplus
}
#endif

#endif /* __BSP_DAC_H */
