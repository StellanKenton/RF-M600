/************************************************************************************
 * @file     : bsp_usart.h
 * @brief    : M600 USART1/USART2 init - ported from M600 HAL
 * @details  : USART1: PA9 TX, PA10 RX, 115200, DMA RX(Ch5)/TX(Ch4). USART2: PA2 TX, PA3 RX, 115200.
 * @hardware : STM32F103xE (M600)
 ***********************************************************************************/
#ifndef __BSP_USART_H
#define __BSP_USART_H

#include "stm32f10x.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_USART_REC_LEN   512u

extern uint8_t BSP_USART1_RxBuf[BSP_USART_REC_LEN];

void BSP_USART1_Init(uint32_t Baud);
void BSP_USART2_Init(uint32_t Baud);

void BSP_USART1_DMA_Send(const uint8_t *pData, uint32_t Len);
void BSP_USART2_Send(const uint8_t *pData, uint32_t Len);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_USART_H */
