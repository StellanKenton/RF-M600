/************************************************************************************
 * @file     : drv_tim.h
 * @brief    : Timer driver - ESW_P/ESW_N via GPIO (PB8/PB9), shockwave compatible.
 ***********************************************************************************/
#ifndef DRV_TIM_H
#define DRV_TIM_H

#include "stm32f10x.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void Drv_TIM4_SetESW_P(bool state);   /* PB8 GPIO: ESW+ */
void Drv_TIM4_SetESW_N(bool state);   /* PB9 GPIO: ESW- */

#ifdef __cplusplus
}
#endif

#endif /* DRV_TIM_H */
