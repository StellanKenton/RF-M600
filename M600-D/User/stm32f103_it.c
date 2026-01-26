/************************************************************************************
 * @file     : stm32f103_it.c
 * @brief    : M600-D interrupt handlers - ported from M600
 * @details  : Cortex fault + DMA1 Ch4/Ch5 (USART1 TX/RX) + DMA1 Ch6/Ch7 (USART2 RX/TX) + USART1/USART2 (IDLE). Std lib.
 ***********************************************************************************/
#include "stm32f103_it.h"
#include "stm32f10x_conf.h"
#include "bsp_delay.h"

/* -----------------------------------------------------------------------------
 * Cortex-M3 exception handlers
 * ----------------------------------------------------------------------------- */

void NMI_Handler(void)
{
    while (1) { }
}

//void HardFault_Handler(void)
//{
//    while (1) { }
//}

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

/* SysTick: 1ms tick for BSP_Delay / BSP_GetTick_ms */
void SysTick_Handler(void)
{
    BSP_SysTick_Inc();
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
 * DMA1 Channel6 (USART2 RX) - clear flags on TC / HT (circular)
 * ----------------------------------------------------------------------------- */
void DMA1_Channel6_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC6) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TC6);
        /* Optional: process full buffer */
    }
    if (DMA_GetITStatus(DMA1_IT_HT6) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_HT6);
        /* Optional: process half buffer */
    }
    if (DMA_GetITStatus(DMA1_IT_TE6) != RESET)
        DMA_ClearITPendingBit(DMA1_IT_TE6);
}

/* -----------------------------------------------------------------------------
 * DMA1 Channel7 (USART2 TX) - clear flags on TC
 * ----------------------------------------------------------------------------- */
void DMA1_Channel7_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC7) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TC7);
        /* Optional: user callback for TX complete */
    }
    if (DMA_GetITStatus(DMA1_IT_TE7) != RESET)
        DMA_ClearITPendingBit(DMA1_IT_TE7);
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

/* -----------------------------------------------------------------------------
 * USART2 - IDLE line (frame end). Clear IDLE; optional DMA restart.
 * ----------------------------------------------------------------------------- */
void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)
    {
        USART_ClearITPendingBit(USART2, USART_IT_IDLE);
        /* Optional: frame end - process BSP_USART2_RxBuf, restart DMA, etc. */
    }
}
