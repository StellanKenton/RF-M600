/************************************************************************************
 * @file     : bsp_tim.c
 * @brief    : M600 TIM1/TIM2 init - ported from M600 HAL
 * @details  : TIM1: external clock ETR(PA12), PWM CH1(PA8)/CH1N(PB13). TIM2: 100us tick. PB8/PB9 as GPIO (see bsp_gpio).
 ***********************************************************************************/
#include "bsp_tim.h"

void BSP_TIM1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_TIM1, ENABLE);

    /* TIM1_ETR: PA12, input */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* TIM1_CH1: PA8, TIM1_CH1N: PB13, AF push-pull */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Time base: prescaler 0, period 4, up count (for ETR clock) */
    TIM_TimeBaseStructure.TIM_Period            = 4 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler         = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision     = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode       = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    /* External clock mode 2: ETR */
    TIM_ITRxExternalClockConfig(TIM1, TIM_TS_ETRF);
    TIM_ETRClockMode2Config(TIM1, TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted, 0);

    /* PWM CH1 / CH1N */
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
    TIM_OCInitStructure.TIM_Pulse       = 2;
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);

    TIM_BDTRInitTypeDef TIM_BDTRInitStructure;
    TIM_BDTRInitStructure.TIM_OSSRState       = TIM_OSSRState_Enable;
    TIM_BDTRInitStructure.TIM_OSSIState       = TIM_OSSIState_Enable;
    TIM_BDTRInitStructure.TIM_LOCKLevel       = TIM_LOCKLevel_1;
    TIM_BDTRInitStructure.TIM_DeadTime        = 8;
    TIM_BDTRInitStructure.TIM_Break           = TIM_Break_Disable;
    TIM_BDTRInitStructure.TIM_BreakPolarity   = TIM_BreakPolarity_High;
    TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Disable;
    TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);

    TIM_ARRPreloadConfig(TIM1, ENABLE);

    /* Do not enable timer/PWM here; call BSP_TIM1_ComplementaryPWM_Enable() when PWM output is needed. */
    TIM_CtrlPWMOutputs(TIM1, DISABLE);
    TIM_Cmd(TIM1, DISABLE);
    
}

void BSP_TIM2_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Enable TIM2 clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* Time base configuration: 100us period
     * Assuming APB1 timer clock = 72MHz (if APB1 prescaler = 1)
     * To get 100us period: 72MHz * 100us = 7200 cycles
     * Prescaler = 720 - 1 = 719 -> timer clock = 72MHz / 720 = 100kHz
     * Period = 10 - 1 = 9 -> interrupt every 10 cycles = 100us
     */
    TIM_TimeBaseStructure.TIM_Period        = 10 - 1;  /* 10 cycles = 100us at 100kHz */
    TIM_TimeBaseStructure.TIM_Prescaler     = 720 - 1; /* 72MHz / 720 = 100kHz */
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /* Enable TIM2 update interrupt */
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    /* Configure NVIC for TIM2 interrupt */
    NVIC_InitStructure.NVIC_IRQChannel                   = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd               = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Enable TIM2 */
    TIM_Cmd(TIM2, ENABLE);
}

void BSP_TIM1_SetCompare1(uint16_t pulse)
{
    TIM_SetCompare1(TIM1, pulse);
}

void BSP_TIM1_ComplementaryPWM_Enable(void)
{
    TIM_Cmd(TIM1, ENABLE);   
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
}

void BSP_TIM1_ComplementaryPWM_Disable(void)
{
    TIM_CtrlPWMOutputs(TIM1, DISABLE);
    TIM_Cmd(TIM1, DISABLE);
}
