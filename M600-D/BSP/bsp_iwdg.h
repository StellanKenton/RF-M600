/************************************************************************************
 * @file     : bsp_iwdg.h
 * @brief    : M600 IWDG module - prescaler 4, reload 4095 (STM32 Standard Library)
 * @details  : Ported from M600 HAL. Tout = (4*4*4095)/40k ¡Ö 1.64s.
 * @hardware : STM32F103xE (M600)
 ***********************************************************************************/
#ifndef __BSP_IWDG_H
#define __BSP_IWDG_H

#include "stm32f10x.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void BSP_IWDG_Init(void);   /* M600 config: Prescaler_4, Reload 4095 */
void BSP_IWDG_Feed(void);

/* Optional: generic init (kept for compatibility) */
void IWDG_Init(uint8_t prer, uint16_t rlr);
void IWDG_Feed(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_IWDG_H */
