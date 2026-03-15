/************************************************************************************
 * @file     : bsp_tim.h
 * @brief    : M600 TIM module - TIM1 (ETR+PWM CH1/CH1N), TIM2 (10us tick)
 * @details  : Ported from M600 HAL. TIM1: PA12 ETR, PA8 CH1, PB13 CH1N. PB8/PB9 as GPIO (ESW_P/ESW_N).
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
void BSP_TIM2_Init(void);   /* TIM2: System tick timer, 10us interrupt, no PWM */

void BSP_TIM1_SetCompare1(uint16_t pulse);
void BSP_TIM1_ComplementaryPWM_Enable(void);   /* enable TIM1 CH1/CH1N complementary PWM output */
void BSP_TIM1_ComplementaryPWM_Disable(void);  /* disable TIM1 CH1/CH1N complementary PWM output */

#ifdef __cplusplus
}
#endif

#endif /* __BSP_TIM_H */
