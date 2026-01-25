/************************************************************************************
 * @file     : drv_wdg.h
 * @brief    : Watchdog driver - DRV API, DAL calls BSP (Std lib)
 ***********************************************************************************/
#ifndef DRV_WDG_H
#define DRV_WDG_H

#include "stm32f10x.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t Drv_WatchDog_Init(void);
void Drv_WatchDogFeed(void);
void Drv_WatchDogResartCheck(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_WDG_H */
