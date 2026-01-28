/************************************************************************************
 * @file     : drv_usart.c
 * @brief    : USART driver - DRV calls DAL, DAL calls BSP (Std lib)
 ***********************************************************************************/
#include "drv_usart.h"
#include "bsp_usart.h"
#include "lib_ringbuffer.h"

CBuff s_USART1_RxBuffer;
CBuff s_USART2_RxBuffer;

uint8_t s_USART1_RxBufferData[BSP_USART_REC_LEN];
uint8_t s_USART2_RxBufferData[BSP_USART_REC_LEN];

void Drv_USART1_Init(void)
{
    CBuff_Init(&s_USART1_RxBuffer, s_USART1_RxBufferData, BSP_USART_REC_LEN);
}

void Drv_USART2_Init(void)
{
    CBuff_Init(&s_USART2_RxBuffer, s_USART2_RxBufferData, BSP_USART_REC_LEN);
}

void Dal_USART1_Send(const uint8_t *pData, uint32_t Len)
{
    BSP_USART1_DMA_Send(pData, Len);
}

void Drv_USART1_Send(const uint8_t *pData, uint32_t Len)
{
    Dal_USART1_Send(pData, Len);
}

void Dal_USART2_Send(const uint8_t *pData, uint32_t Len)
{
    BSP_USART2_DMA_Send(pData, Len);
}

void Drv_USART2_Send(const uint8_t *pData, uint32_t Len)
{
    Dal_USART2_Send(pData, Len);
}

bool Dal_GetUSART1_DMA_SendStatus(void)
{
    return (bool)BSP_USART1_DMA_TxStatus();
}

bool Dal_GetUSART2_DMA_SendStatus(void)
{
    return (bool)BSP_USART2_DMA_TxStatus();
}

bool Drv_GetUSART1_DMA_SendStatus(void)
{
    return (bool)Dal_GetUSART1_DMA_SendStatus();
}

bool Drv_GetUSART2_DMA_SendStatus(void)
{
    return (bool)Dal_GetUSART2_DMA_SendStatus();
}

void Drv_Uart_init()
{
    Drv_USART1_Init();
    Drv_USART2_Init();
}

void Drv_USART1_Rx(void)
{
    // 停止DMA接收（防止数据被覆盖）
    DMA_Cmd(DMA1_Channel5, DISABLE);
    
    // 等待DMA停止
    while (DMA1_Channel5->CCR & DMA_CCR5_EN) { }
    
    // 计算已接收的数据长度
    // DMA1_Channel5是循环模式，当前计数器表示剩余未接收的字节数
    uint16_t dmaCounter = DMA_GetCurrDataCounter(DMA1_Channel5);
    uint16_t rxLen = BSP_USART_REC_LEN - dmaCounter;
    
    // 如果有数据，将BSP层接收缓冲区的数据写入DRV层环形缓冲区
    if (rxLen > 0)
    {
        CBuff_Write(&s_USART1_RxBuffer, BSP_USART1_RxBuf, rxLen);
    }
    
    // 重新启动DMA接收（循环模式）
    DMA_SetCurrDataCounter(DMA1_Channel5, BSP_USART_REC_LEN);
    DMA_Cmd(DMA1_Channel5, ENABLE);
}

void Drv_USART2_Rx(void)
{
    // 停止DMA接收（防止数据被覆盖）
    DMA_Cmd(DMA1_Channel6, DISABLE);
    
    // 等待DMA停止
    while (DMA1_Channel6->CCR & DMA_CCR6_EN) { }
    
    // 计算已接收的数据长度
    // DMA1_Channel6是循环模式，当前计数器表示剩余未接收的字节数
    uint16_t dmaCounter = DMA_GetCurrDataCounter(DMA1_Channel6);
    uint16_t rxLen = BSP_USART_REC_LEN - dmaCounter;
    if (rxLen > 0)
    {
        CBuff_Write(&s_USART2_RxBuffer, BSP_USART2_RxBuf, rxLen);
    }
    
    // 重新启动DMA接收（循环模式）
    DMA_SetCurrDataCounter(DMA1_Channel6, BSP_USART_REC_LEN);
    DMA_Cmd(DMA1_Channel6, ENABLE);
}


CBuff* Drv_GetUsart1RingPtr(void)
{
    return &s_USART1_RxBuffer;
}

CBuff* Drv_GetUsart2RingPtr(void)
{
    return &s_USART2_RxBuffer;
}

