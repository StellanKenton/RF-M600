/************************************************************************************
 * @file     : drv_usart.c
 * @brief    : USART driver - DRV calls DAL, DAL calls BSP (Std lib)
 ***********************************************************************************/
#include "drv_usart.h"
#include "bsp_usart.h"

static void Dal_USART2_Send(const uint8_t *pData, uint32_t Len)
{
    BSP_USART2_Send(pData, Len);
}

void Drv_USART2_Send(const uint8_t *pData, uint32_t Len)
{
    Dal_USART2_Send(pData, Len);
}
