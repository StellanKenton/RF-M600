/************************************************************************************
 * @file     : bsp_delay.c
 * @brief    : M600 delay and tick - Unified timer using TIM2
 * @details  : Uses TIM2 (10us interrupt) and shared global time variable.
 ***********************************************************************************/
#include "bsp_delay.h"

/* Global system time in microseconds (updated by TIM2 interrupt every 10us) */
volatile uint64_t g_SystemTimeUs = 0;

void BSP_SysTick_Init(void)
{
    /* TIM2 initialization is done in BSP_TIM2_Init, called from BSP_Init */
    /* This function is kept for compatibility but does nothing */
    g_SystemTimeUs = 0;
}

void BSP_SysTick_Inc(void)
{
    /* This function is no longer used - TIM2 interrupt updates g_SystemTimeUs directly */
}

void BSP_Delay_ms(uint32_t ms)
{
    uint64_t start = g_SystemTimeUs;
    uint64_t delay_us = (uint64_t)ms * 1000;
    while ((g_SystemTimeUs - start) < delay_us) {
        __NOP();
    }
}

uint32_t BSP_GetTick_ms(void)
{
    return (uint32_t)(g_SystemTimeUs / 1000);
}
