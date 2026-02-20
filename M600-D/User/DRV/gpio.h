/************************************************************************************
 * @file     : gpio.h
 * @brief    : HAL GPIO compatibility layer for lib_aiic (STM32 Standard Library)
 * @details  : Maps HAL_GPIO_WritePin/ReadPin to STM32 StdLib GPIO functions
 ***********************************************************************************/
#ifndef GPIO_H
#define GPIO_H

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* HAL GPIO_PinState compatibility */
typedef uint8_t GPIO_PinState;
#define GPIO_PIN_RESET  0
#define GPIO_PIN_SET    1

static inline void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
    if (PinState)
        GPIO_SetBits(GPIOx, GPIO_Pin);
    else
        GPIO_ResetBits(GPIOx, GPIO_Pin);
}

static inline GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    return (GPIO_PinState)GPIO_ReadInputDataBit(GPIOx, GPIO_Pin);
}

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H */
