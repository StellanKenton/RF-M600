/************************************************************************************
 * @file     : bsp_iwdg.c
 * @brief    : M600 IWDG init - ported from M600 HAL
 * @details  : Prescaler 4, Reload 4095. LSI 40kHz -> Tout �� 1.64s.
 ***********************************************************************************/
#include "bsp_iwdg.h"

void BSP_IWDG_Init(void)
{
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(IWDG_Prescaler_4);
    IWDG_SetReload(4095);
    IWDG_ReloadCounter();
    IWDG_Enable();
}

void BSP_IWDG_Feed(void)
{
    IWDG_ReloadCounter();
}
