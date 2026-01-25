/************************************************************************************
 * @file     : drv_init.c
 * @brief    : System init - DRV calls DAL, DAL calls BSP (Std lib)
 ***********************************************************************************/
#include "drv_init.h"
#include "bsp_gpio.h"
#include "drv_wdg.h"

static void Dal_System_Init(void)
{
    BSP_Init();
}

void Drv_System_Init(void)
{
    Dal_System_Init();
    Drv_WatchDog_Init();
}
