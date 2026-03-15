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
#define pwr_control5_Pin     GPIO_Pin_12
#define pwr_control5_Port    GPIOC
#define pwr_control4_Pin     GPIO_Pin_2
#define pwr_control4_Port    GPIOD
#define pwr_control3_Pin     GPIO_Pin_3
#define pwr_control3_Port    GPIOB
#define pwr_control2_Pin     GPIO_Pin_4
#define pwr_control2_Port    GPIOB
#define pwr_control1_Pin     GPIO_Pin_5
#define pwr_control1_Port    GPIOB
#define MCU_CTR_OUT_Pin      GPIO_Pin_14
#define MCU_CTR_OUT_Port     GPIOB
#define MCU_CTR_US_RF_Pin    GPIO_Pin_12
#define MCU_CTR_US_RF_Port   GPIOB
#define CTR_HP_motor_Pin     GPIO_Pin_7
#define CTR_HP_motor_Port    GPIOC
#define CTR_HP_lose_Pin      GPIO_Pin_8
#define CTR_HP_lose_Port     GPIOC
#define CTR_FAN_HP_Pin       GPIO_Pin_9
#define CTR_FAN_HP_Port      GPIOC
#define CTR_FAN_Pin          GPIO_Pin_6
#define CTR_FAN_Port         GPIOC
#define ESW_P_Pin            GPIO_Pin_9   /* PB9, shockwave ESW+ */
#define ESW_P_Port           GPIOB
#define ESW_N_Pin            GPIO_Pin_8   /* PB8, shockwave ESW- */
#define ESW_N_Port           GPIOB
#define LED_PIN              GPIO_Pin_15
#define LED_PORT             GPIOC

/* Inputs */
#define MCU_FOOT_Pin         GPIO_Pin_14
#define MCU_FOOT_Port        GPIOC
#define IO_SYN_US_Pin        GPIO_Pin_15
#define IO_SYN_US_Port       GPIOA
#define IO_SYN_RF_Pin        GPIO_Pin_10
#define IO_SYN_RF_Port       GPIOC
#define IO_SYN_ESW_Pin       GPIO_Pin_11
#define IO_SYN_ESW_Port      GPIOC
#define MCU_I_O_Pin          GPIO_Pin_1
#define MCU_I_O_Port         GPIOC

/* Helper macros */
#define MCU_Buzzer_ON()      GPIO_ResetBits(MCU_Buzzer_Port, MCU_Buzzer_Pin)
#define MCU_Buzzer_OFF()     GPIO_SetBits(MCU_Buzzer_Port, MCU_Buzzer_Pin)
#define CTR_FAN_ON()         GPIO_SetBits(CTR_FAN_Port, CTR_FAN_Pin)
#define CTR_FAN_OFF()        GPIO_ResetBits(CTR_FAN_Port, CTR_FAN_Pin)
#define MCU_LED_ON()         GPIO_SetBits(MCU_LED_Port, MCU_LED_Pin)
#define MCU_LED_OFF()        GPIO_ResetBits(MCU_LED_Port, MCU_LED_Pin)

/* BSP GPIO pin enums for ReadPin/WritePin (DAL maps DRV enums to these) */
typedef enum {
    E_GPIO_IN_FOOT = 0,
    E_GPIO_IN_SYN_US,
    E_GPIO_IN_SYN_RF,
    E_GPIO_IN_SYN_ESW,
    E_GPIO_IN_MAX
} GPIO_Input_EnumDef;

typedef enum {
    E_GPIO_OUT_BUZZER = 0,
    E_GPIO_OUT_CTR_US_RF,
    E_GPIO_OUT_CTR_OUT,
    E_GPIO_OUT_MCU_IO,
    E_GPIO_OUT_PWR_CTRL1,
    E_GPIO_OUT_PWR_CTRL2,
    E_GPIO_OUT_PWR_CTRL3,
    E_GPIO_OUT_PWR_CTRL4,
    E_GPIO_OUT_PWR_CTRL5,
    E_GPIO_OUT_CTR_FAN,
    E_GPIO_OUT_CTR_HP_MOTOR,
    E_GPIO_OUT_CTR_HP_LOSE,
    E_GPIO_OUT_CTR_HEAT_HP,
    E_GPIO_OUT_ESW_P,   /* PB8, shockwave ESW+ */
    E_GPIO_OUT_ESW_N,   /* PB9, shockwave ESW- */
    E_GPIO_OUT_LED,     /* PC15, status LED */
    E_GPIO_OUT_MAX
} GPIO_Output_EnumDef;

void BSP_GPIO_Init(void);
uint8_t BSP_GPIO_ReadPin(GPIO_Input_EnumDef pin);   /* 0=low, 1=high */
void BSP_GPIO_WritePin(GPIO_Output_EnumDef pin, uint8_t state);  /* 0=low, 1=high */

/** Call all M600 BSP inits: GPIO, ADC, TIM1, TIM2, USART1(115200), USART2(115200). PB8/PB9 as GPIO (ESW_P/ESW_N). */
void BSP_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_GPIO_H */
