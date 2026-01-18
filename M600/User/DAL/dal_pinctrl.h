/************************************************************************************
* @file     : dal_pinctrl.h
* @brief    : 
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef DAL_PINCTRL_H
#define DAL_PINCTRL_H

#include <string.h>
#include <stdbool.h>
#include "stdint.h"

#ifdef __cplusplus
#include <iostream>
extern "C" {
#endif

/**
 * @brief GPIO Output pins enumeration (输出引脚枚举)
 */
 typedef enum
 {
     E_GPIO_OUT_BUZZER = 0,           ///< MCU_Buzzer - 蜂鸣器
     E_GPIO_OUT_CTR_US_RF,            ///< MCU_CTR_US_RF - 超声/射频控制 (PB12)
     E_GPIO_OUT_CTR_OUT,              ///< MCU_CTR_OUT - 输出控制 (PB12)
     E_GPIO_OUT_MCU_IO,               ///< MCU_I_O - IO控制 (PB15)
     E_GPIO_OUT_PWR_CTRL1,            ///< pwr_control1 - 功率控制1
     E_GPIO_OUT_PWR_CTRL2,            ///< pwr_control2 - 功率控制2
     E_GPIO_OUT_PWR_CTRL3,            ///< pwr_control3 - 功率控制3
     E_GPIO_OUT_PWR_CTRL4,            ///< pwr_control4 - 功率控制4
     E_GPIO_OUT_CTR_FAN,              ///< CTR_FAN - 风扇控制
     E_GPIO_OUT_CTR_HP_MOTOR,         ///< CTR_HP_motor - HP电机控制
     E_GPIO_OUT_CTR_HP_LOSE,          ///< CTR_HP_lose - HP释放控制
     E_GPIO_OUT_CTR_HEAT_HP,          ///< CTR_HEAT_HP - HP加热控制
     E_GPIO_OUT_MAX                   ///< 输出引脚数量
 } GPIO_Output_EnumDef;
 
 /**
  * @brief GPIO Input pins enumeration (输入引脚枚举)
  */
 typedef enum
 {
     E_GPIO_IN_FOOT = 0,              ///< MCU_FOOT - 脚踏开关
     E_GPIO_IN_SYN_US,                ///< IO_SYN_US - 超声同步信号
     E_GPIO_IN_SYN_RF,                ///< IO_SYN_RF - 射频同步信号
     E_GPIO_IN_SYN_ESW,               ///< IO_SYN_ESW - 冲击波同步信号
     E_GPIO_IN_MAX                    ///< 输入引脚数量
 } GPIO_Input_EnumDef;


 bool Dal_Read_Pin(GPIO_Input_EnumDef pin);
 void Dal_Write_Pin(GPIO_Output_EnumDef pin, uint8_t state);

#ifdef __cplusplus
}
#endif
#endif  // DAL_PINCTRL_H
/**************************End of file********************************/

