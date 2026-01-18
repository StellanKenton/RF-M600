/***********************************************************************************
* @file     : dal_pinctrl.c
* @brief    : 
* @details  : 
* @author   : \.rumi
* @date     : 2025-01-23
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "dal_pinctrl.h"
#include "stm32f1xx_hal.h"
#include "main.h"


bool Dal_Read_Pin(GPIO_Input_EnumDef pin)
{
    switch (pin)
    {
        case E_GPIO_IN_FOOT:
            return HAL_GPIO_ReadPin((GPIO_TypeDef*)MCU_FOOT_GPIO_Port, MCU_FOOT_Pin);
        case E_GPIO_IN_SYN_US:
            return HAL_GPIO_ReadPin((GPIO_TypeDef*)IO_SYN_US_GPIO_Port, IO_SYN_US_Pin);
        case E_GPIO_IN_SYN_RF:
            return HAL_GPIO_ReadPin((GPIO_TypeDef*)IO_SYN_RF_GPIO_Port, IO_SYN_RF_Pin);
        case E_GPIO_IN_SYN_ESW:
            return HAL_GPIO_ReadPin((GPIO_TypeDef*)IO_SYN_ESW_GPIO_Port, IO_SYN_ESW_Pin);
    }
    return false;
}

void Dal_Write_Pin(GPIO_Output_EnumDef pin, uint8_t state)
{
    switch (pin)
    {
        case E_GPIO_OUT_BUZZER:
            HAL_GPIO_WritePin((GPIO_TypeDef*)MCU_Buzzer_GPIO_Port, MCU_Buzzer_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_CTR_US_RF:
            HAL_GPIO_WritePin((GPIO_TypeDef*)MCU_CTR_US_RF_GPIO_Port, MCU_CTR_US_RF_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_CTR_OUT:
            HAL_GPIO_WritePin((GPIO_TypeDef*)MCU_CTR_OUT_GPIO_Port, MCU_CTR_OUT_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_MCU_IO:
            HAL_GPIO_WritePin((GPIO_TypeDef*)MCU_I_O_GPIO_Port, MCU_I_O_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_PWR_CTRL1:
            HAL_GPIO_WritePin((GPIO_TypeDef*)pwr_control1_GPIO_Port, pwr_control1_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_PWR_CTRL2:
            HAL_GPIO_WritePin((GPIO_TypeDef*)pwr_control2_GPIO_Port, pwr_control2_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_PWR_CTRL3:
            HAL_GPIO_WritePin((GPIO_TypeDef*)pwr_control3_GPIO_Port, pwr_control3_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_PWR_CTRL4:
            HAL_GPIO_WritePin((GPIO_TypeDef*)pwr_control4_GPIO_Port, pwr_control4_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_CTR_FAN:
            HAL_GPIO_WritePin((GPIO_TypeDef*)CTR_FAN_GPIO_Port, CTR_FAN_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_CTR_HP_MOTOR:
            HAL_GPIO_WritePin((GPIO_TypeDef*)CTR_HP_motor_GPIO_Port, CTR_HP_motor_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_CTR_HP_LOSE:
            HAL_GPIO_WritePin((GPIO_TypeDef*)CTR_HP_lose_GPIO_Port, CTR_HP_lose_Pin, (GPIO_PinState)state);
            break;
        case E_GPIO_OUT_CTR_HEAT_HP:
            HAL_GPIO_WritePin((GPIO_TypeDef*)CTR_HEAT_HP_GPIO_Port, CTR_HEAT_HP_Pin, (GPIO_PinState)state);
            break;
        default:
            break;
    }
}



/**************************End of file********************************/


