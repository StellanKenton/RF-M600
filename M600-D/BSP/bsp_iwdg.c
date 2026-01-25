/************************************************************************************
 * @file     : bsp_iwdg.c
 * @brief    : M600 IWDG init - ported from M600 HAL
 * @details  : Prescaler 4, Reload 4095. LSI 40kHz -> Tout ¡Ö 1.64s.
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

void IWDG_Init(uint8_t prer, uint16_t rlr)
{
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(prer);
    IWDG_SetReload(rlr);
    IWDG_ReloadCounter();
    IWDG_Enable();
    IWDG_ReloadCounter();
}

void IWDG_Feed(void)
{
    IWDG_ReloadCounter();
}
