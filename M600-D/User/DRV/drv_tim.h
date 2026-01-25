/************************************************************************************
 * @file     : drv_tim.h
 * @brief    : Timer driver - DRV API, DAL calls BSP (Std lib). TIM4 CH3/CH4 for ESW.
 ***********************************************************************************/
#ifndef DRV_TIM_H
#define DRV_TIM_H

#include "stm32f10x.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void Drv_TIM4_SetCompare3(uint16_t pulse);
void Drv_TIM4_SetCompare4(uint16_t pulse);
void Drv_TIM4_SetESW_P(bool state);   /* TIM4_CH3: high = 65535, low = 0 */
void Drv_TIM4_SetESW_N(bool state);   /* TIM4_CH4: high = 65535, low = 0 */

#ifdef __cplusplus
}
#endif

#endif /* DRV_TIM_H */
