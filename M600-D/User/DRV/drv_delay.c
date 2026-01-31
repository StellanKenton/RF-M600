/************************************************************************************
 * @file     : drv_delay.c
 * @brief    : Delay/tick driver - Unified timer using TIM2, single global time variable
 ***********************************************************************************/
#include "drv_delay.h"
#include "bsp_delay.h"
#include "stm32f10x.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* Use external global time variable from bsp_delay.c */
extern volatile uint64_t g_SystemTimeUs;

void Dal_Delay(uint32_t ms)
{
    uint64_t start = g_SystemTimeUs;
    uint64_t delay_us = (uint64_t)ms * 1000;
    while ((g_SystemTimeUs - start) < delay_us) {
        __NOP();
    }
}

uint32_t Dal_GetTick(void)
{
    return (uint32_t)(g_SystemTimeUs / 1000);
}

void Drv_SysTick_Increment(void)
{
    g_SystemTimeUs += SYSTEM_TICK_PER_SECOND;
}

uint64_t Drv_GetSystemTickUs(void)
{
    return g_SystemTimeUs;
}

uint64_t Drv_GetSystemTickMs(void)
{
    return g_SystemTimeUs / 1000;
}

uint32_t Drv_Delay_GetTickMs(void)
{
    return (uint32_t)(g_SystemTimeUs / 1000);
}

void Drv_Delay_ms(uint32_t ms)
{
    uint64_t start = g_SystemTimeUs;
    uint64_t delay_us = (uint64_t)ms * 1000;
    while ((g_SystemTimeUs - start) < delay_us) {
        __NOP();
    }
}

bool Drv_Timer_Tick(Drv_Timer_t *pTimer, uint32_t timeout_ms)
{
    if (pTimer == NULL)
        return false;
    uint32_t now = Dal_GetTick();
    if (!pTimer->running) {
        pTimer->start_ms = now;
        pTimer->timeout_ms = timeout_ms;
        pTimer->running = true;
        return false;
    }
    if ((now - pTimer->start_ms) >= pTimer->timeout_ms) {
        pTimer->start_ms = now;
        pTimer->timeout_ms = timeout_ms;
        return true;
    }
    return false;
}
