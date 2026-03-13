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

/* DC-DC voltage control (centivolt: voltage * 100, e.g. 1500 = 15.00V) */
#define DRV_DAC_DCDC_CENTIVOLT_MIN    600u   /* 6.00V */
#define DRV_DAC_DCDC_CENTIVOLT_MAX  3600u   /* 36.00V */
#define DRV_DAC_DCDC_CENTIVOLT_DEFAULT 1500u /* 15.00V */

void Drv_DAC_SetVoltage(uint16_t centivolt);
uint16_t Drv_DAC_GetVoltage(void);
void Drv_DAC_Init(void);
uint16_t Drv_DAC_GetDCDCVoltageCentivolt(void);
#ifdef __cplusplus
}   
#endif

#endif /* DRV_DAC_H */
