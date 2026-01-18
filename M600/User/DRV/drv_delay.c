/**
* Copyright (c) 2023, AstroCeta, Inc. All rights reserved.
* \file drv_delay.h
* \brief Implementation of a ring buffer for efficient data handling.
* \date 2025-07-30
* \author AstroCeta, Inc.
**/
#include "drv_delay.h"
#include "stm32f1xx_hal.h"


static uint64_t s_SystemTick = 0;

#ifdef OS_PLATFORM
void Drv_osDelay(uint32_t ms)
{
    osDelay(ms);
}
#endif


void Drv_Delay(uint32_t ms)
{
    HAL_Delay(ms);
}

uint32_t Drv_GetTick(void)
{
    return HAL_GetTick();
}



void Drv_SysTick_Increment(void)
{
    s_SystemTick+=SYSTEM_TICK_PER_SECOND;
}


uint64_t Drv_GetSystemTickUs(void)
{
    return s_SystemTick;
}

uint64_t Drv_GetSystemTickMs(void)
{
    return s_SystemTick/1000;
}

/**
 * @brief 定时器计时函数（自动启动和检查超时）
 * @param pTimer 定时器结构体指针
 * @param timeout_ms 超时时间 (毫秒)
 * @return true: 定时时间已到达, false: 定时时间未到达
 * @note 首次调用自动启动定时器，超时后自动重新启动
 */
bool Drv_Timer_Tick(Drv_Timer_t *pTimer, uint32_t timeout_ms)
{
    if (pTimer == NULL) return false;
    
    uint64_t current_time = Drv_GetSystemTickUs();
    uint64_t timeout_us = (uint64_t)timeout_ms * 1000;
    
    // 如果定时器未运行，启动它
    if (!pTimer->running)
    {
        pTimer->start_time = current_time;
        pTimer->timeout = timeout_us;
        pTimer->running = true;
        return false;
    }
    
    // 检查是否超时
    if ((current_time - pTimer->start_time) >= pTimer->timeout)
    {
        pTimer->start_time = current_time;  // 重新启动
        pTimer->timeout = timeout_us;
        return true;  // 超时，返回1
    }
    
    return false;  // 未超时，返回0
}

/**************************End of file********************************/


