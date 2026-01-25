/************************************************************************************
 * @file     : drv_usart.h
 * @brief    : USART driver - DRV API, DAL calls BSP (Std lib)
 ***********************************************************************************/
#ifndef DRV_USART_H
#define DRV_USART_H

#include "stm32f10x.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Drv_USART2_Send(const uint8_t *pData, uint32_t Len);

#ifdef __cplusplus
}
#endif

#endif /* DRV_USART_H */
