/************************************************************************************
 * @file     : drv_wdg.c
 * @brief    : Watchdog driver - DRV calls DAL, DAL calls BSP (Std lib)
 ***********************************************************************************/
#include "drv_wdg.h"
#include "bsp_iwdg.h"
#include "stm32f10x_rcc.h"

static void Dal_WDG_Init(void)
{
    BSP_IWDG_Init();
}

static void Dal_WDG_Feed(void)
{
    BSP_IWDG_Feed();
}

uint8_t Drv_WatchDog_Init(uint8_t data)
{
    (void)data;
    Dal_WDG_Init();
    return 0;
}

void Drv_WatchDogFeed(void)
{
    Dal_WDG_Feed();
}

void Drv_WatchDogResartCheck(void)
{
    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET) {
        RCC_ClearFlag();
    }
}
