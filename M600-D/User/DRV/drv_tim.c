/************************************************************************************
 * @file     : drv_tim.c
 * @brief    : Timer driver - ESW_P/ESW_N via GPIO (PB8/PB9), no TIM4
 ***********************************************************************************/
#include "drv_tim.h"
#include "bsp_gpio.h"

void Drv_TIM4_SetESW_P(bool state)
{
    BSP_GPIO_WritePin(BSP_GPIO_OUT_ESW_P, state ? 1 : 0);
}

void Drv_TIM4_SetESW_N(bool state)
{
    BSP_GPIO_WritePin(BSP_GPIO_OUT_ESW_N, state ? 1 : 0);
}
