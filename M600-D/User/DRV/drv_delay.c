/************************************************************************************
 * @file     : drv_delay.c
 * @brief    : Delay/tick driver - DRV calls DAL, DAL calls BSP (Std lib)
 ***********************************************************************************/
#include "drv_delay.h"
#include "bsp_delay.h"
#include <stdint.h>
#include <string.h>

static uint64_t s_SystemTick = 0;

void Dal_Delay(uint32_t ms)
{
    BSP_Delay_ms(ms);
}

uint32_t Dal_GetTick(void)
{
    return BSP_GetTick_ms();
}

void Drv_SysTick_Increment(void)
{
    s_SystemTick += SYSTEM_TICK_PER_SECOND;
}

uint64_t Drv_GetSystemTickUs(void)
{
    return s_SystemTick;
}

uint64_t Drv_GetSystemTickMs(void)
{
    return s_SystemTick / 1000;
}

uint32_t Drv_Delay_GetTickMs(void)
{
    return Dal_GetTick();
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
