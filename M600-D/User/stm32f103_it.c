/************************************************************************************
 * @file     : stm32f103_it.c
 * @brief    : M600-D interrupt handlers - ported from M600
 * @details  : Cortex fault + DMA1 Ch4/Ch5 (USART1 TX/RX) + DMA1 Ch6/Ch7 (USART2 RX/TX) + USART1/USART2 (IDLE). Std lib.
 ***********************************************************************************/
#include "stm32f103_it.h"
#include "stm32f10x_conf.h"
#include "bsp_delay.h"
#include "drv_usart.h"
#include "bsp_adc.h"
#include "drv_delay.h"
#include "app_shockwave.h"
#include "app_ultrasound.h"
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

/* TIM2: 100us interrupt for unified system time */
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        Drv_SysTick_Increment();  /* Updates g_SystemTimeUs by 100us */
        App_Shockwave_TimerTick100us();
        App_Ultrasound_SetHighFreqPowerHandle();
    }
}

/* -----------------------------------------------------------------------------
 * DMA1 Channel1 (ADC1) - double buffering on TC
 * ----------------------------------------------------------------------------- */
void DMA1_Channel1_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC1) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TC1);
        /* Copy DMA working buffer to read buffer (double buffering) */
        BSP_ADC_DMA_TC_Handler();
    }
    if (DMA_GetITStatus(DMA1_IT_TE1) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TE1);
        /* Error handling - optional */
    }
}

/* -----------------------------------------------------------------------------
 * DMA1 Channel4 (USART1 TX) - clear flags on TC
 * ----------------------------------------------------------------------------- */
void DMA1_Channel4_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC4) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TC4);
        /* 发送完成后关闭TX DMA通道，否则EN位会保持�?1，�?�致TxStatus一直显示busy */
        DMA_Cmd(DMA1_Channel4, DISABLE);
        while (DMA1_Channel4->CCR & DMA_CCR4_EN) { }
        /* Optional: user callback for TX complete */
    }
    if (DMA_GetITStatus(DMA1_IT_TE4) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TE4);
        /* 出错时也关闭通道，避免一直busy */
        DMA_Cmd(DMA1_Channel4, DISABLE);
        while (DMA1_Channel4->CCR & DMA_CCR4_EN) { }
    }
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
        /* 必须先�?�取 USART_DR 寄存器来清除 IDLE �?�?标志 */
        /* 即使数据已经�? DMA 读取，也需要�?�取一次来清除 IDLE 标志 */
        volatile uint16_t temp = USART1->DR;
        (void)temp;  /* 避免编译器�?�告 */
        
        /* 然后处理接收到的数据 */
        Drv_USART1_Rx();
        
        /* 清除 IDLE �?�?标志（虽然�?�取 DR 后标志应该已经清除，但为了保险还�?清除一下） */
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
        /* 必须先�?�取 USART_DR 寄存器来清除 IDLE �?�?标志 */
        /* 即使数据已经�? DMA 读取，也需要�?�取一次来清除 IDLE 标志 */
        volatile uint16_t temp = USART2->DR;
        (void)temp;  /* 避免编译器�?�告 */
        
        /* 清除 IDLE �?�?标志（虽然�?�取 DR 后标志应该已经清除，但为了保险还�?清除一下） */
        USART_ClearITPendingBit(USART2, USART_IT_IDLE);
        
        /* 然后处理接收到的数据 */
        Drv_USART2_Rx();
        /* Optional: frame end - process BSP_USART2_RxBuf, restart DMA, etc. */
    }
}
