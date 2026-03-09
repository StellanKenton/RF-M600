/************************************************************************************
 * @file     : bsp_adc.h
 * @brief    : M600 ADC BSP - 负责获取原始ADC值
 * @details  : ADC1 DMA连续采集，提供原始ADC数值读取接口
 * @hardware : STM32F103xE (M600)
 ***********************************************************************************/
#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "stm32f10x.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* M600 ADC通道定义 (ADC1): PA0(0), PA1(1), PA5(5), PA6(6), PB0(8), PB1(9), PC2(12), PC3(13), PC5(15) */
typedef enum {
    BSP_ADC_CH_US_I = 0,       /* 超声电流 - ADC1_IN0  PA0 */
    BSP_ADC_CH_RF_I,           /* 射频电流 - ADC1_IN1  PA1 */
    BSP_ADC_CH_Heat_REF02,     /* 加热参考2 - ADC1_IN5  PA5 */
    BSP_ADC_CH_Heat_REF01,     /* 加热参考1/主板NTC - ADC1_IN6  PA6 */
    BSP_ADC_CH_ESW_U,          /* ESW电压 - ADC1_IN8  PB0 */
    BSP_ADC_CH_ESW_I,          /* ESW电流 - ADC1_IN9  PB1 */
    BSP_ADC_CH_HP_PRE,         /* 高压预压 - ADC1_IN12 PC2 */
    BSP_ADC_CH_HAND_NTC,       /* 手柄NTC - ADC1_IN13 PC3 */
    BSP_ADC_CH_VOUT,           /* 输出电压 - ADC1_IN15 PC5 */
    BSP_ADC_CH_MAX
} BSP_ADC_Channel_t;

#define BSP_ADC_REF_MV        3300u
#define BSP_ADC_RESOLUTION    4096u

/**
 * @brief  初始化ADC模块
 */
void BSP_ADC_Init(void);

/**
 * @brief  读取指定通道的原始ADC值
 * @param  ch: ADC通道
 * @retval 原始ADC值 (0-4095)
 */
uint16_t BSP_ADC_ReadChannel(BSP_ADC_Channel_t ch);

/**
 * @brief  DMA传输完成中断处理函数 (从中断中调用)
 */
void BSP_ADC_DMA_TC_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_ADC_H */
