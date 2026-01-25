/************************************************************************************
 * @file     : bsp_delay.c
 * @brief    : M600 SysTick delay and tick - Std lib
 * @details  : 1ms period SysTick, BSP_Delay_ms, BSP_GetTick_ms.
 ***********************************************************************************/
#include "bsp_delay.h"

static volatile uint32_t s_tick_ms = 0;

void BSP_SysTick_Init(void)
{
    /* SysTick 1ms: SystemCoreClock typically 72MHz */
    if (SysTick_Config(SystemCoreClock / 1000) != 0) {
        while (1) { }
    }
    s_tick_ms = 0;
}

void BSP_SysTick_Inc(void)
{
    s_tick_ms++;
}

void BSP_Delay_ms(uint32_t ms)
{
    uint32_t start = s_tick_ms;
    while ((s_tick_ms - start) < ms) {
        __NOP();
    }
}

uint32_t BSP_GetTick_ms(void)
{
    return s_tick_ms;
}
