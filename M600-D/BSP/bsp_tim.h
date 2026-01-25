/************************************************************************************
 * @file     : bsp_tim.h
 * @brief    : M600 TIM module - TIM1 (ETR+PWM CH1/CH1N), TIM4 (PWM CH3/CH4)
 * @details  : Ported from M600 HAL. TIM1: PA12 ETR, PA8 CH1, PB13 CH1N. TIM4: PB8 CH3, PB9 CH4.
 * @hardware : STM32F103xE (M600)
 ***********************************************************************************/
#ifndef __BSP_TIM_H
#define __BSP_TIM_H

#include "stm32f10x.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void BSP_TIM1_Init(void);   /* TIM1: ETR(PA12), CH1(PA8), CH1N(PB13), PWM, period 65535 */
void BSP_TIM4_Init(void);   /* TIM4: CH3(PB8), CH4(PB9), PWM, period 65535 */

void BSP_TIM1_SetCompare1(uint16_t pulse);
void BSP_TIM4_SetCompare3(uint16_t pulse);
void BSP_TIM4_SetCompare4(uint16_t pulse);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_TIM_H */
