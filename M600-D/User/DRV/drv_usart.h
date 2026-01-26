/************************************************************************************
 * @file     : drv_usart.h
 * @brief    : USART driver - DRV API, DAL calls BSP (Std lib)
 ***********************************************************************************/
#ifndef DRV_USART_H
#define DRV_USART_H

#include "stm32f10x.h"
#include <stdint.h>
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif
#define DRV_USART_REC_LEN    1024u
#define DRV_USART_SEND_LEN   512u


void Drv_USART1_Send(const uint8_t *pData, uint32_t Len);
void Drv_USART2_Send(const uint8_t *pData, uint32_t Len);
void Drv_USART1_Rx(void);
bool Drv_GetUSART1_DMA_SendStatus(void);
bool Drv_GetUSART2_DMA_SendStatus(void);
void Drv_Uart_init(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_USART_H */
