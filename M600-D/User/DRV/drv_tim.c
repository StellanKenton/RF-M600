/************************************************************************************
 * @file     : drv_tim.c
 * @brief    : Timer driver - DRV calls DAL, DAL calls BSP (Std lib)
 ***********************************************************************************/
#include "drv_tim.h"
#include "bsp_tim.h"

#define BSP_TIM4_PERIOD  65535u

static void Dal_TIM4_SetCompare3(uint16_t pulse)
{
    BSP_TIM4_SetCompare3(pulse);
}

static void Dal_TIM4_SetCompare4(uint16_t pulse)
{
    BSP_TIM4_SetCompare4(pulse);
}

void Drv_TIM4_SetCompare3(uint16_t pulse)
{
    Dal_TIM4_SetCompare3(pulse);
}

void Drv_TIM4_SetCompare4(uint16_t pulse)
{
    Dal_TIM4_SetCompare4(pulse);
}

void Drv_TIM4_SetESW_P(bool state)
{
    Dal_TIM4_SetCompare3(state ? BSP_TIM4_PERIOD : 0);
}

void Drv_TIM4_SetESW_N(bool state)
{
    Dal_TIM4_SetCompare4(state ? BSP_TIM4_PERIOD : 0);
}
