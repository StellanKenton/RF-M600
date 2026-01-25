/************************************************************************************
 * @file     : bsp_gpio.h
 * @brief    : M600 GPIO module - pin definitions and init (STM32 Standard Library)
 * @details  : Ported from M600 HAL. Pins per M600 main.h / gpio.c
 * @hardware : STM32F103xE (M600)
 ***********************************************************************************/
#ifndef __BSP_GPIO_H
#define __BSP_GPIO_H

#include "stm32f10x.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- M600 pin definitions (from main.h) ---------- */
/* Outputs */
#define MCU_Buzzer_Pin       GPIO_Pin_13
#define MCU_Buzzer_Port      GPIOC
#define pwr_control4_Pin     GPIO_Pin_6
#define pwr_control4_Port    GPIOC
#define pwr_control3_Pin     GPIO_Pin_7
#define pwr_control3_Port    GPIOC
#define pwr_control2_Pin     GPIO_Pin_8
#define pwr_control2_Port    GPIOC
#define pwr_control1_Pin     GPIO_Pin_9
#define pwr_control1_Port    GPIOC
#define MCU_CTR_OUT_Pin      GPIO_Pin_12
#define MCU_CTR_OUT_Port     GPIOB
#define MCU_CTR_US_RF_Pin    GPIO_Pin_14
#define MCU_CTR_US_RF_Port   GPIOB
#define CTR_HP_motor_Pin     GPIO_Pin_3
#define CTR_HP_motor_Port    GPIOB
#define CTR_HP_lose_Pin      GPIO_Pin_4
#define CTR_HP_lose_Port     GPIOB
#define CTR_HEAT_HP_Pin      GPIO_Pin_5
#define CTR_HEAT_HP_Port     GPIOB
#define CTR_FAN_Pin          GPIO_Pin_2
#define CTR_FAN_Port         GPIOD

/* Inputs */
#define MCU_FOOT_Pin         GPIO_Pin_14
#define MCU_FOOT_Port        GPIOC
#define IO_SYN_US_Pin        GPIO_Pin_10
#define IO_SYN_US_Port       GPIOC
#define IO_SYN_RF_Pin        GPIO_Pin_11
#define IO_SYN_RF_Port       GPIOC
#define IO_SYN_ESW_Pin       GPIO_Pin_12
#define IO_SYN_ESW_Port      GPIOC
#define MCU_I_O_Pin          GPIO_Pin_15
#define MCU_I_O_Port         GPIOB

/* Helper macros */
#define MCU_Buzzer_ON()      GPIO_ResetBits(MCU_Buzzer_Port, MCU_Buzzer_Pin)
#define MCU_Buzzer_OFF()     GPIO_SetBits(MCU_Buzzer_Port, MCU_Buzzer_Pin)
#define CTR_FAN_ON()         GPIO_SetBits(CTR_FAN_Port, CTR_FAN_Pin)
#define CTR_FAN_OFF()        GPIO_ResetBits(CTR_FAN_Port, CTR_FAN_Pin)

/* BSP GPIO pin enums for ReadPin/WritePin (DAL maps DRV enums to these) */
typedef enum {
    BSP_GPIO_IN_FOOT = 0,
    BSP_GPIO_IN_SYN_US,
    BSP_GPIO_IN_SYN_RF,
    BSP_GPIO_IN_SYN_ESW,
    BSP_GPIO_IN_MAX
} BSP_GPIO_Input_t;

typedef enum {
    BSP_GPIO_OUT_BUZZER = 0,
    BSP_GPIO_OUT_CTR_US_RF,
    BSP_GPIO_OUT_CTR_OUT,
    BSP_GPIO_OUT_MCU_IO,
    BSP_GPIO_OUT_PWR_CTRL1,
    BSP_GPIO_OUT_PWR_CTRL2,
    BSP_GPIO_OUT_PWR_CTRL3,
    BSP_GPIO_OUT_PWR_CTRL4,
    BSP_GPIO_OUT_CTR_FAN,
    BSP_GPIO_OUT_CTR_HP_MOTOR,
    BSP_GPIO_OUT_CTR_HP_LOSE,
    BSP_GPIO_OUT_CTR_HEAT_HP,
    BSP_GPIO_OUT_MAX
} BSP_GPIO_Output_t;

void BSP_GPIO_Init(void);
uint8_t BSP_GPIO_ReadPin(BSP_GPIO_Input_t pin);   /* 0=low, 1=high */
void BSP_GPIO_WritePin(BSP_GPIO_Output_t pin, uint8_t state);  /* 0=low, 1=high */

/** Call all M600 BSP inits: GPIO, ADC, TIM1, TIM4, USART1(115200), USART2(115200). */
void BSP_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_GPIO_H */
