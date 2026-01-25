/************************************************************************************
 * @file     : bsp_adc.h
 * @brief    : M600 ADC module - ADC1 init and channel read (STM32 Standard Library)
 * @details  : Ported from M600 HAL. ADC1 channels: PA0,1,5,6 / PB0,1 / PC2,3.
 * @hardware : STM32F103xE (M600)
 ***********************************************************************************/
#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "stm32f10x.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* M600 ADC channels (ADC1): PA0(0), PA1(1), PA5(5), PA6(6), PB0(8), PB1(9), PC2(12), PC3(13) */
typedef enum {
    BSP_ADC_CH_US_I = 0,       /* ADC1_IN0  PA0 */
    BSP_ADC_CH_RF_I,           /* ADC1_IN1  PA1 */
    BSP_ADC_CH_Heat_REF02,     /* ADC1_IN5  PA5 */
    BSP_ADC_CH_Heat_REF01,     /* ADC1_IN6  PA6 */
    BSP_ADC_CH_ESW_U,          /* ADC1_IN8  PB0 */
    BSP_ADC_CH_ESW_I,          /* ADC1_IN9  PB1 */
    BSP_ADC_CH_HP_PRE,         /* ADC1_IN12 PC2 */
    BSP_ADC_CH_HAND_NTC,       /* ADC1_IN13 PC3 */
    BSP_ADC_CH_MAX
} BSP_ADC_Channel_t;

#define BSP_ADC_REF_MV        3300u
#define BSP_ADC_RESOLUTION    4096u

void BSP_ADC_Init(void);
uint16_t BSP_ADC_ReadChannel(BSP_ADC_Channel_t ch);
uint32_t BSP_ADC_ReadVoltage(BSP_ADC_Channel_t ch);
const uint16_t* BSP_ADC_GetDmaBuffer(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_ADC_H */
