/**
* Copyright (c) 2023, AstroCeta, Inc. All rights reserved.
* \file drv_delay.h
* \brief Implementation of a ring buffer for efficient data handling.
* \date 2025-07-30
* \author AstroCeta, Inc.
**/
#ifndef DRV_DELAY_H
#define DRV_DELAY_H

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
#include <iostream>
extern "C" {
#endif


#define SYSTEM_TICK_PER_SECOND  100    // us

//#define OS_PLATFORM 

void Dal_Delay(uint32_t ms);
uint32_t Dal_GetTick(void);
#ifdef OS_PLATFORM
void Drv_osDelay(uint32_t ms);
#endif


typedef struct
{
    uint64_t start_time;    // 开始时间 (us)
    uint64_t timeout;       // 超时时间 (us)
    bool     running;       // 定时器是否在运行
} Drv_Timer_t;

void Drv_SysTick_Increment(void);
uint64_t Drv_GetSystemTickUs(void);
uint64_t Drv_GetSystemTickMs(void);
bool Drv_Timer_Tick(Drv_Timer_t *pTimer, uint32_t timeout_ms);


#ifdef __cplusplus
}
#endif
#endif  // DRV_DELAY_H
/**************************End of file********************************/

