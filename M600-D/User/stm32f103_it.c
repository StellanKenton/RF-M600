/************************************************************************************
 * @file     : stm32f103_it.c
 * @brief    : M600-D interrupt handlers - ported from M600
 * @details  : Cortex fault + DMA1 Ch4/Ch5 (USART1 TX/RX) + USART1 (IDLE). Std lib.
 ***********************************************************************************/
#include "stm32f103_it.h"
#include "stm32f10x_conf.h"

/* -----------------------------------------------------------------------------
 * Cortex-M3 exception handlers
 * ----------------------------------------------------------------------------- */

void NMI_Handler(void)
{
    while (1) { }
}

void HardFault_Handler(void)
{
    while (1) { }
}

void MemManage_Handler(void)
{
    while (1) { }
}

void BusFault_Handler(void)
{
    while (1) { }
}

void UsageFault_Handler(void)
{
    while (1) { }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

/* SysTick: delay.c uses polling, no IRQ. Leave weak or empty. */
void SysTick_Handler(void)
{
}

/* -----------------------------------------------------------------------------
 * DMA1 Channel4 (USART1 TX) - clear flags on TC
 * ----------------------------------------------------------------------------- */
void DMA1_Channel4_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC4) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TC4);
        /* Optional: user callback for TX complete */
    }
    if (DMA_GetITStatus(DMA1_IT_TE4) != RESET)
        DMA_ClearITPendingBit(DMA1_IT_TE4);
}

/* -----------------------------------------------------------------------------
 * DMA1 Channel5 (USART1 RX) - clear flags on TC / HT (circular)
 * ----------------------------------------------------------------------------- */
void DMA1_Channel5_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC5) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TC5);
        /* Optional: process full buffer */
    }
    if (DMA_GetITStatus(DMA1_IT_HT5) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_HT5);
        /* Optional: process half buffer */
    }
    if (DMA_GetITStatus(DMA1_IT_TE5) != RESET)
        DMA_ClearITPendingBit(DMA1_IT_TE5);
}

/* -----------------------------------------------------------------------------
 * USART1 - IDLE line (frame end). Clear IDLE; optional DMA restart.
 * ----------------------------------------------------------------------------- */
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
    {
        USART_ClearITPendingBit(USART1, USART_IT_IDLE);
        /* Optional: frame end - process BSP_USART1_RxBuf, restart DMA, etc. */
    }
}
