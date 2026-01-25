/************************************************************************************
 * @file     : drv_dac.h
 * @brief    : DAC driver - DRV API, DAL calls BSP (Std lib)
 ***********************************************************************************/
#ifndef DRV_DAC_H
#define DRV_DAC_H

#include "stm32f10x.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool Drv_DAC_SetVoltage(uint16_t voltage_mv);
uint16_t Drv_DAC_GetVoltage(void);
uint16_t Drv_DAC_GetActualVoltage(void);
void Drv_DAC_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_DAC_H */
