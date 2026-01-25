/************************************************************************************
 * @file     : bsp_delay.h
 * @brief    : M600 SysTick-based delay and tick (STM32 Standard Library)
 * @details  : 1ms SysTick IRQ, BSP_Delay_ms, BSP_GetTick_ms. BSP only.
 * @hardware : STM32F103xE (M600-D)
 ***********************************************************************************/
#ifndef __BSP_DELAY_H
#define __BSP_DELAY_H

#include "stm32f10x.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** SysTick 1ms init. Call from BSP_Init. */
void BSP_SysTick_Init(void);

/** Increment tick. Call from SysTick_Handler only. */
void BSP_SysTick_Inc(void);

/** Blocking delay, milliseconds. */
void BSP_Delay_ms(uint32_t ms);

/** Get tick count in milliseconds (since BSP_SysTick_Init). */
uint32_t BSP_GetTick_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_DELAY_H */
