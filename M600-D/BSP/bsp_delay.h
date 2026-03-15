/************************************************************************************
 * @file     : bsp_delay.h
 * @brief    : M600 delay and tick - Unified timer using TIM2
 * @details  : Uses TIM2 (10us interrupt) and shared global time variable.
 * @hardware : STM32F103xE (M600-D)
 ***********************************************************************************/
#ifndef __BSP_DELAY_H
#define __BSP_DELAY_H

#include "stm32f10x.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Global system time in microseconds (defined in bsp_delay.c) */
extern volatile uint64_t g_SystemTimeUs;

/** Timer init (for compatibility, does nothing - TIM2 is initialized separately). */
void BSP_SysTick_Init(void);

/** Increment tick (deprecated - TIM2 interrupt handles this). */
void BSP_SysTick_Inc(void);

/** Blocking delay, milliseconds. */
void BSP_Delay_ms(uint32_t ms);

/** Get tick count in milliseconds (since BSP_SysTick_Init). */
uint32_t BSP_GetTick_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_DELAY_H */
