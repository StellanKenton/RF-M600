/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MCU_Buzzer_Pin GPIO_PIN_13
#define MCU_Buzzer_GPIO_Port GPIOC
#define MCU_FOOT_Pin GPIO_PIN_14
#define MCU_FOOT_GPIO_Port GPIOC
#define MCU_CTR_OUT_Pin GPIO_PIN_12
#define MCU_CTR_OUT_GPIO_Port GPIOB
#define MCU_CTR_US_RF_Pin GPIO_PIN_14
#define MCU_CTR_US_RF_GPIO_Port GPIOB
#define MCU_I_O_Pin GPIO_PIN_15
#define MCU_I_O_GPIO_Port GPIOB
#define pwr_control4_Pin GPIO_PIN_6
#define pwr_control4_GPIO_Port GPIOC
#define pwr_control3_Pin GPIO_PIN_7
#define pwr_control3_GPIO_Port GPIOC
#define pwr_control2_Pin GPIO_PIN_8
#define pwr_control2_GPIO_Port GPIOC
#define pwr_control1_Pin GPIO_PIN_9
#define pwr_control1_GPIO_Port GPIOC
#define IO_SYN_US_Pin GPIO_PIN_10
#define IO_SYN_US_GPIO_Port GPIOC
#define IO_SYN_RF_Pin GPIO_PIN_11
#define IO_SYN_RF_GPIO_Port GPIOC
#define IO_SYN_ESW_Pin GPIO_PIN_12
#define IO_SYN_ESW_GPIO_Port GPIOC
#define CTR_FAN_Pin GPIO_PIN_2
#define CTR_FAN_GPIO_Port GPIOD
#define CTR_HP_motor_Pin GPIO_PIN_3
#define CTR_HP_motor_GPIO_Port GPIOB
#define CTR_HP_lose_Pin GPIO_PIN_4
#define CTR_HP_lose_GPIO_Port GPIOB
#define CTR_HEAT_HP_Pin GPIO_PIN_5
#define CTR_HEAT_HP_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
