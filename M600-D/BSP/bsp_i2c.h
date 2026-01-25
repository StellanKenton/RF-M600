/************************************************************************************
 * @file     : bsp_i2c.h
 * @brief    : M600 I2C module - I2C1(PB6/7), I2C2(PB10/11), 100kHz (STM32 Standard Library)
 * @details  : Ported from M600 HAL. Hardware I2C, 7-bit addr, duty cycle 2.
 * @hardware : STM32F103xE (M600)
 ***********************************************************************************/
#ifndef __BSP_I2C_H
#define __BSP_I2C_H

#include "stm32f10x.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void BSP_I2C1_Init(void);
void BSP_I2C2_Init(void);

/* Polled master transmit: 7-bit dev addr, then buf[len]. Returns 0 on success. */
int BSP_I2C1_Transmit(uint8_t devAddr, const uint8_t *buf, uint16_t len);
int BSP_I2C1_Receive(uint8_t devAddr, uint8_t *buf, uint16_t len);
int BSP_I2C2_Transmit(uint8_t devAddr, const uint8_t *buf, uint16_t len);
int BSP_I2C2_Receive(uint8_t devAddr, uint8_t *buf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_I2C_H */
